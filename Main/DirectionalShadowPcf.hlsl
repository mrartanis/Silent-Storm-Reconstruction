// Appended to the actual psShadowTestSmoothed translation. The existing vertex
// shader, two packed depth channels, bias, shadow colour and output alpha stay
// compatible. Filter comparison results, not interpolated depth values.
float4 main(Varying i) : SV_TARGET {
    if (u_pixelRegisters[2].z < 0.5) return legacyShadow(i);
    float4 receiver = saturate(i.color0);
    float3 channels = u_pixelRegisters[0].rgb;
    float4 shadow = u_pixelRegisters[1];
    float2 uv = i.tex0.xy / i.tex0.w;
    uint width, height;
    s0Texture.GetDimensions(width, height);
    float2 size = float2(width, height);
    int2 center = int2(floor(uv * size));
    float2 f = frac(uv * size);
    // Continuous three-texel quadratic tent weights in each axis.
    float3 wx = float3(0.5*(1-f.x)*(1-f.x), 0.75-(f.x-0.5)*(f.x-0.5), 0.5*f.x*f.x);
    float3 wy = float3(0.5*(1-f.y)*(1-f.y), 0.75-(f.y-0.5)*(f.y-0.5), 0.5*f.y*f.y);
    // Correct receiver depth at each tap on sloped surfaces. No extra constant
    // bias is needed; the legacy one-quantum comparison threshold is retained.
    float depth = dot(receiver.rgb, channels) / max(dot(channels, 1.0.xxx), 1.0);
    float2 dx = ddx(uv), dy = ddy(uv);
    float dzdx = ddx(depth), dzdy = ddy(depth);
    float det = dx.x*dy.y - dx.y*dy.x;
    float2 gradient = 0;
    if (abs(det) > 1e-12)
        gradient = float2(dzdx*dy.y-dzdy*dx.y, dx.x*dzdy-dy.x*dzdx) / det;
    float coverage = 0;
    [unroll] for (int y=-1; y<=1; ++y) {
        [unroll] for (int x=-1; x<=1; ++x) {
            int2 tap = clamp(center+int2(x,y), int2(0,0), int2(width-1,height-1));
            float3 mapDepth = s0Texture.Load(int3(tap,0)).rgb;
            float correction = dot(gradient, (float2(tap)+0.5)/size-uv);
            float3 tapReceiver = saturate(receiver.rgb + correction);
            float test = 4*dot(saturate(mapDepth-tapReceiver), channels) + shadow.a;
            coverage += (test > 0.5 ? 1.0 : 0.0) * wx[x+1] * wy[y+1];
        }
    }
    float4 r0 = float4(lerp(1.0.xxx, shadow.rgb, coverage), receiver.a+saturate(i.color1).a);
    if (u_alphaTest.y > 0.5) {
        float a=r0.a, ref=u_alphaTest.x, func=u_alphaTest.z;
        bool alphaPass=(func==8)||(func==2 && a<ref)||(func==3 && a==ref)||(func==4 && a<=ref)||
                  (func==5 && a>ref)||(func==6 && a!=ref)||(func==7 && a>=ref);
        if (!alphaPass) discard;
    }
    return r0;
}
