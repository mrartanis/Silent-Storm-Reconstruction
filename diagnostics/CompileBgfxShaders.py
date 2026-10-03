"""Translate the game's actual shader-model-1 tokens to HLSL and compile with bgfx shaderc.

Debug comments are deliberately ignored: several reconstructed shaders have retail
bytecode corrections. Unsupported instructions fail the build instead of drawing
with a replacement shader. Generated sources/binaries belong in the build tree.
"""
import argparse
import concurrent.futures
import itertools
from pathlib import Path
import re
import struct
import subprocess

OPS = {1: ('mov', 2), 2: ('add', 3), 3: ('sub', 3), 4: ('mad', 4),
       5: ('mul', 3), 6: ('rcp', 2), 7: ('rsq', 2), 8: ('dp3', 3),
       9: ('dp4', 3), 10: ('min', 3), 11: ('max', 3), 12: ('slt', 3),
       13: ('sge', 3), 18: ('lrp', 4), 20: ('m4x4', 3), 31: ('dcl', 2),
       64: ('texcrd', 1), 66: ('texld', 1), 69: ('texreg2ar', 2),
       71: ('texm3x2pad', 2), 72: ('texm3x2tex', 2),
       73: ('texm3x3pad', 2), 77: ('texm3x3vspec', 2), 80: ('cnd', 4),
       81: ('def', 5), 0xfffd: ('phase', 0)}


def decode(tokens):
    minor = tokens[0] & 255
    pos = 1
    result = []
    while pos < len(tokens):
        instruction = tokens[pos]
        pos += 1
        opcode = instruction & 65535
        if opcode == 65535:
            if pos != len(tokens):
                raise ValueError('Unexpected data after END')
            return result
        if opcode == 65534:
            pos += (instruction >> 16) & 32767
            continue
        if opcode not in OPS:
            raise ValueError(f'Unsupported opcode {opcode}')
        name, count = OPS[opcode]
        if minor == 4 and opcode in (64, 66):
            count = 2
        args = tokens[pos:pos + count]
        if len(args) != count:
            raise ValueError('Truncated shader instruction')
        result.append((name, args, bool(instruction & 0x40000000)))
        pos += count
    raise ValueError('Shader missing END')


def register(token, vertex):
    kind = ((token >> 28) & 7) | ((token >> 8) & 24)
    num = token & 2047
    if kind == 0:
        return f'r{num}'
    if kind == 1:
        return f'v{num}'
    if kind == 2:
        return f'c[{num}]'
    if kind == 3 and not vertex:
        return f't{num}'
    if kind == 4 and num == 0:
        return 'o.position'
    if kind == 5:
        return f'o.color{num}'
    if kind == 6:
        return f'o.tex{num}'
    raise ValueError(f'Unsupported register {kind}:{num}')


def source(token, vertex):
    value = register(token, vertex)
    swizzle = ''.join('xyzw'[(token >> (16 + k * 2)) & 3] for k in range(4))
    value += '.' + swizzle
    modifier = (token >> 24) & 15
    expressions = {
        0: value, 1: f'(-{value})', 2: f'({value}-0.5)', 3: f'(-({value}-0.5))',
        4: f'({value}*2.0-1.0)', 5: f'(1.0-{value}*2.0)',
        6: f'(1.0-{value})', 7: f'({value}*2.0)', 8: f'(-{value}*2.0)',
        9: f'({value}/{register(token, vertex)}.z)',
        10: f'({value}/{register(token, vertex)}.w)',
    }
    if modifier not in expressions:
        raise ValueError(f'Unsupported source modifier {modifier}')
    return expressions[modifier]


VARYING = ('struct Varying { float4 position : SV_POSITION; '
           'float4 color0 : COLOR0; float4 color1 : COLOR1; ' +
           ' '.join(f'float4 tex{k} : TEXCOORD{k};' for k in range(6)) + ' };\n')


def translate(tokens, cube_mask=0):
    vertex = tokens[0] >> 16 == 0xfffe
    instructions = decode(tokens)
    samplers = set()
    code = []
    matrix_rows = []
    for index, (op, args, coissue) in enumerate(instructions):
        if op in ('dcl', 'phase'):
            continue
        if coissue and (not index or instructions[index - 1][2]):
            raise ValueError('Invalid coissue pair')
        dst = args[0]
        dreg = register(dst, vertex)
        mask = ''.join('xyzw'[k] for k in range(4) if dst & (1 << (16 + k)))
        if not mask:
            raise ValueError('Empty destination mask')
        src = [source(a, vertex) for a in args[1:]] if op != 'def' else []
        if op == 'def':
            values = struct.unpack('<4f', struct.pack('<4I', *args[1:]))
            expr = 'float4(' + ','.join(f'{v:.9g}' for v in values) + ')'
        elif op == 'mov': expr = src[0]
        elif op == 'add': expr = f'({src[0]}+{src[1]})'
        elif op == 'sub': expr = f'({src[0]}-{src[1]})'
        elif op == 'mul': expr = f'({src[0]}*{src[1]})'
        elif op == 'mad': expr = f'({src[0]}*{src[1]}+{src[2]})'
        elif op in ('min', 'max'): expr = f'{op}({src[0]},{src[1]})'
        elif op == 'lrp': expr = f'lerp({src[2]},{src[1]},{src[0]})'
        elif op in ('slt', 'sge'):
            expr = f'float4({src[0]} {"<" if op == "slt" else ">="} {src[1]})'
        elif op == 'rcp': expr = f'(1.0/({src[0]}).x).xxxx'
        elif op == 'rsq': expr = f'rsqrt(abs(({src[0]}).x)).xxxx'
        elif op in ('dp3', 'dp4'):
            comp = 'xyz' if op == 'dp3' else 'xyzw'
            expr = f'dot(({src[0]}).{comp},({src[1]}).{comp}).xxxx'
        elif op == 'm4x4':
            base = args[2] & 2047
            expr = 'float4(' + ','.join(f'dot({src[0]}, c[{base+k}])' for k in range(4)) + ')'
        elif op == 'cnd':
            # PS 1.1 tests src0 alpha for the entire vector; PS 1.4 is componentwise.
            test = src[0] if (tokens[0] & 255) == 4 else f'({src[0]}).aaaa'
            expr = f'lerp({src[2]},{src[1]},float4({test}>0.5))'
        elif op == 'texcrd':
            expr = src[0] if src else f'saturate(i.tex{dst & 2047})'
        elif op in ('texm3x2pad', 'texm3x3pad'):
            matrix_rows.append(f'dot(i.tex{dst & 2047}.xyz, ({src[0]}).xyz)')
            continue
        elif op in ('texld', 'texreg2ar', 'texm3x2tex', 'texm3x3vspec'):
            # PS 1.4 binds the sampler by destination, independently of coordinates.
            stage = dst & 2047
            samplers.add(stage)
            cube = bool(cube_mask & (1 << stage))
            if op == 'texreg2ar':
                coord = f'float4(({src[0]}).w,({src[0]}).x,0,1)'
            elif op == 'texm3x2tex':
                if len(matrix_rows) != 1: raise ValueError('Invalid texm3x2 sequence')
                coord = f'float4({matrix_rows[0]},dot(i.tex{stage}.xyz,({src[0]}).xyz),0,1)'
                matrix_rows.clear()
            elif op == 'texm3x3vspec':
                if len(matrix_rows) != 2: raise ValueError('Invalid texm3x3 sequence')
                normal = f'float3({matrix_rows[0]},{matrix_rows[1]},dot(i.tex{stage}.xyz,({src[0]}).xyz))'
                eye = f'float3(i.tex{stage-2}.w,i.tex{stage-1}.w,i.tex{stage}.w)'
                coord = f'float4(2.0*dot({normal},{eye})/dot({normal},{normal})*{normal}-{eye},1)'
                matrix_rows.clear()
            else:
                coord = src[0] if src else f'(i.tex{stage}/lerp(1.0,i.tex{stage}.w,u_projectionFlags[{stage//4}].{"xyzw"[stage%4]}))'
            expr = f's{stage}Texture.Sample(s{stage}Sampler,({coord}).{"xyz" if cube else "xy"})'
        else:
            raise ValueError(f'Unsupported operation {op}')
        shift = (dst >> 24) & 15
        if shift:
            exponent = shift if shift <= 3 else shift - 16
            if not -3 <= exponent <= 3: raise ValueError('Invalid destination shift')
            expr = f'({expr}*{2.0**exponent})'
        if dst & (1 << 20): expr = f'saturate({expr})'
        # Evaluate both members before either write: coissued alpha can read the
        # previous RGB value of the same destination register.
        statement = f'float4 value{index} = {expr};'
        write = f'{dreg}.{mask} = value{index}.{mask};'
        if coissue:
            prev = code.pop()
            code.extend([statement, prev, write])
        else:
            code.extend([statement, write])
    uniform = 'u_vertexRegisters' if vertex else 'u_pixelRegisters'
    count = 96 if vertex else 8
    prefix = f'uniform float4 {uniform}[{count}];\n' + VARYING
    if vertex:
        # SPIR-V reflection maps these exact names to bgfx vertex attributes.
        # Struct inputs become "i.position" and lose the bgfx attribute mapping.
        prefix += ('uniform float4 u_rasterOffset;\n'
                   'Varying main(float3 a_position : POSITION, float4 a_normal : NORMAL, '
                   'float2 a_texcoord0 : TEXCOORD0, float2 a_texcoord1 : TEXCOORD1, '
                   'float4 a_color0 : COLOR0, float4 a_tangent : TANGENT, '
                   'float4 a_bitangent : BITANGENT) { Varying o = (Varying)0; '
                   'float4 v0=float4(a_position,1), v1=a_normal, v2=a_color0, '
                   'v3=float4(a_texcoord0,0,1), v6=float4(a_texcoord1,0,1), '
                   'v4=a_tangent, v5=a_bitangent;\n')
    else:
        prefix += 'uniform float4 u_alphaTest;\nuniform float4 u_projectionFlags[2];\n'
        for stage in sorted(samplers):
            kind = 'TextureCube' if cube_mask & (1 << stage) else 'Texture2D'
            prefix += (f'{kind}<float4> s{stage}Texture : register(t{stage});\n'
                       f'SamplerState s{stage}Sampler : register(s{stage});\n')
        prefix += 'float4 main(Varying i) : SV_TARGET { float4 v0=saturate(i.color0),v1=saturate(i.color1);\n'
        prefix += ''.join(f'float4 t{k}=i.tex{k};\n' for k in range(6))
    prefix += f'float4 c[{count}];\n' + ''.join(f'c[{k}]={uniform}[{k}];\n' for k in range(count))
    prefix += ''.join(f'float4 r{k}=0;\n' for k in range(12))
    ending = 'o.position.xy += u_rasterOffset.xy * o.position.w; return o; }\n' if vertex else ('if (u_alphaTest.y > 0.5) { float a=r0.a, ref=u_alphaTest.x, f=u_alphaTest.z; '
              'bool alphaPass=(f==8)||(f==2 && a<ref)||(f==3 && a==ref)||(f==4 && a<=ref)||'
              '(f==5 && a>ref)||(f==6 && a!=ref)||(f==7 && a>=ref); if (!alphaPass) discard; } return r0; }\n')
    return prefix + '\n'.join(code) + '\n' + ending, samplers


def corpus(path):
    text = path.read_text()
    arrays = {}
    for name, body in re.findall(r'static DWORD (\w+)\[\]\s*=\s*\{(.*?)\};', text, re.S):
        body = re.sub(r'/\*.*?\*/|//[^\n]*', '', body, flags=re.S)
        arrays[name] = [int(x.strip(), 0) for x in body.split(',') if x.strip()]
    entries = []
    for name, number, array in re.findall(r'SVShader (\w+)\(\s*(\d+),\s*(\w+)\s*\)', text):
        entries.append((name, int(number), arrays[array]))
    for name, number, a, b in re.findall(r'SPShader (\w+)\(\s*(\d+),\s*(\w+),\s*(\w+)', text):
        entries.append((name, int(number), arrays[b if b != '0' else a]))
    return entries


def compile_all(args):
    out = Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    (out.parent / 'varying.def.sc').write_text('')
    jobs = []
    for name, number, tokens in corpus(Path(args.source)):
        _, samplers = translate(tokens)
        masks = [sum(1 << s for s in selected)
                 for count in range(len(samplers) + 1)
                 for selected in itertools.combinations(sorted(samplers), count)]
        for mask in masks:
            hlsl, _ = translate(tokens, mask)
            if name == 'psShadowTestSmoothed' and mask == 0:
                # Explicit bgfx quality extension; retain the token translation
                # as a runtime reference path, never as an unsupported fallback.
                hlsl = hlsl.replace('float4 main(Varying i)', 'float4 legacyShadow(Varying i)')
                hlsl += (Path(args.source).parent / 'DirectionalShadowPcf.hlsl').read_text()
            jobs.append((name, number, mask, hlsl, name.startswith('vs')))
    # Present from a linear BGRA render target; apply the configured game gamma.
    present_vs = (VARYING + 'Varying main(float3 a_position:POSITION,float2 a_texcoord0:TEXCOORD0) {'
                  ' Varying o=(Varying)0;o.position=float4(a_position,1);o.tex0=float4(a_texcoord0,0,1);return o;}')
    present_fs = VARYING + r'''
uniform float4 u_present; // gamma exponent, FXAA enabled, reciprocal width/height
Texture2D<float4> s0Texture:register(t0);
SamplerState s0Sampler:register(s0);
float3 sampleColor(float2 uv) { return s0Texture.Sample(s0Sampler,uv).rgb; }
float luma(float3 color) { return dot(color,float3(0.299,0.587,0.114)); }
float3 filterEdges(float2 uv, float3 center) {
    float2 pixel=u_present.zw;
    float nw=luma(sampleColor(uv+float2(-1,-1)*pixel));
    float ne=luma(sampleColor(uv+float2( 1,-1)*pixel));
    float sw=luma(sampleColor(uv+float2(-1, 1)*pixel));
    float se=luma(sampleColor(uv+float2( 1, 1)*pixel));
    float mid=luma(center);
    float low=min(mid,min(min(nw,ne),min(sw,se)));
    float high=max(mid,max(max(nw,ne),max(sw,se)));
    if(high-low < max(0.0312,high*0.125)) return center;
    float2 direction=float2(-((nw+ne)-(sw+se)),(nw+sw)-(ne+se));
    float reduce=max((nw+ne+sw+se)*(0.25*0.125),1.0/128.0);
    direction=clamp(direction/(min(abs(direction.x),abs(direction.y))+reduce),-8.0,8.0)*pixel;
    float3 a=0.5*(sampleColor(uv+direction*(1.0/3.0-0.5))+
                  sampleColor(uv+direction*(2.0/3.0-0.5)));
    float3 b=a*0.5+0.25*(sampleColor(uv-direction*0.5)+sampleColor(uv+direction*0.5));
    float lb=luma(b);
    return (lb<low || lb>high) ? a : b;
}
float4 main(Varying i):SV_TARGET {
    float4 c=s0Texture.Sample(s0Sampler,i.tex0.xy);
    if(u_present.y>0.5) c.rgb=filterEdges(i.tex0.xy,c.rgb);
    return float4(pow(max(c.rgb,0),u_present.xxx),c.a);
}
'''
    jobs += [('vsPresent', 0, 0, present_vs, True), ('psPresent', 0, 0, present_fs, False)]

    def compile_one(job):
        name, number, mask, hlsl, vertex = job
        stem = f'{name}_{mask}'
        source_path = out.parent / (stem + '.hlsl')
        binary_path = out.parent / (stem + '.bin')
        # shaderc's raw SPIR-V uniform parser requires LF even on a Windows host.
        source_path.write_text(hlsl, encoding='utf-8', newline='\n')
        cmd = [args.shaderc, '-f', str(source_path), '-o', str(binary_path),
               '--type', 'vertex' if vertex else 'fragment', '--platform', args.platform,
               '-p', args.profile, '--raw', '-O', '3']
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(stem + '\n' + result.stdout + result.stderr)
        return stem, number, mask, vertex, binary_path.read_bytes()

    with concurrent.futures.ThreadPoolExecutor(max_workers=12) as pool:
        compiled = list(pool.map(compile_one, jobs))
    lines = ['// Generated by CompileBgfxShaders.py; do not edit.', '#pragma once',
             'namespace S2GameShaders {',
             'struct Binary { int id, cubeMask; bool vertex; const unsigned char* data; unsigned size; };']
    for name, _, _, _, data in compiled:
        lines.append(f'static const unsigned char {name}[]={{' + ','.join(str(x) for x in data) + '};')
    lines.append('static const Binary all[] = {')
    for name, number, mask, vertex, data in compiled:
        lines.append(f'{{{number},{mask},{str(vertex).lower()},{name},{len(data)}}},')
    lines += ['};', '}']
    out.write_text('\n'.join(lines))
    print(f'Compiled {len(compiled)} bgfx shader variants from actual game tokens')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', required=True)
    parser.add_argument('--shaderc', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--platform', default='windows')
    parser.add_argument('--profile', default='s_5_0')
    compile_all(parser.parse_args())
