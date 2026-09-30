#include "StdAfx.h"
#undef for
#include "BgfxBackend.h"
#include "GfxShaders.h"
#include "GfxShadersDescr.h"
#include "GameShaders.h"
#include "../Misc/Win32Helper.h"
#undef min
#undef max
#include <array>
#include <map>
#include <limits>
#include <stdexcept>
#include <cstdio>

namespace NGfx {
namespace {
unsigned activeEpoch = 0, nextEpoch = 0;
bgfx::TextureFormat::Enum TextureFormat(D3DFORMAT format) {
  switch (format) {
  case D3DFMT_A8R8G8B8: case D3DFMT_X8R8G8B8: return bgfx::TextureFormat::BGRA8;
  case D3DFMT_R5G6B5: return bgfx::TextureFormat::B5G6R5;
  case D3DFMT_A1R5G5B5: case D3DFMT_X1R5G5B5: return bgfx::TextureFormat::BGR5A1;
  case D3DFMT_A4R4G4B4: case D3DFMT_X4R4G4B4: return bgfx::TextureFormat::BGRA4;
  case D3DFMT_DXT1: return bgfx::TextureFormat::BC1;
  case D3DFMT_DXT2: case D3DFMT_DXT3: return bgfx::TextureFormat::BC2;
  case D3DFMT_DXT4: case D3DFMT_DXT5: return bgfx::TextureFormat::BC3;
  case D3DFMT_D24S8: return bgfx::TextureFormat::D24S8;
  default: throw std::runtime_error("Unsupported game texture format");
  }
}
unsigned BlockSize(D3DFORMAT format) { return format >= D3DFMT_DXT1 && format <= D3DFMT_DXT5 ? 4 : 1; }
unsigned BytesPerBlock(D3DFORMAT format) {
  if (BlockSize(format) == 4) return format == D3DFMT_DXT1 ? 8 : 16;
  return format == D3DFMT_A8R8G8B8 || format == D3DFMT_X8R8G8B8 || format == D3DFMT_D24S8 ? 4 : 2;
}
template<class T> void Assign(NWin32Helper::com_ptr<T>* target, T* value) { *target = value; }
uint64_t Blend(DWORD value) {
  switch (value) {
  case D3DBLEND_ZERO: return BGFX_STATE_BLEND_ZERO;
  case D3DBLEND_ONE: return BGFX_STATE_BLEND_ONE;
  case D3DBLEND_SRCCOLOR: return BGFX_STATE_BLEND_SRC_COLOR;
  case D3DBLEND_INVSRCCOLOR: return BGFX_STATE_BLEND_INV_SRC_COLOR;
  case D3DBLEND_SRCALPHA: return BGFX_STATE_BLEND_SRC_ALPHA;
  case D3DBLEND_INVSRCALPHA: return BGFX_STATE_BLEND_INV_SRC_ALPHA;
  case D3DBLEND_DESTALPHA: return BGFX_STATE_BLEND_DST_ALPHA;
  case D3DBLEND_INVDESTALPHA: return BGFX_STATE_BLEND_INV_DST_ALPHA;
  case D3DBLEND_DESTCOLOR: return BGFX_STATE_BLEND_DST_COLOR;
  case D3DBLEND_INVDESTCOLOR: return BGFX_STATE_BLEND_INV_DST_COLOR;
  default: throw std::runtime_error("Unsupported game blend factor");
  }
}
uint64_t Depth(DWORD value) {
  switch (value) {
  case D3DCMP_LESS: return BGFX_STATE_DEPTH_TEST_LESS;
  case D3DCMP_EQUAL: return BGFX_STATE_DEPTH_TEST_EQUAL;
  case D3DCMP_LESSEQUAL: return BGFX_STATE_DEPTH_TEST_LEQUAL;
  case D3DCMP_GREATER: return BGFX_STATE_DEPTH_TEST_GREATER;
  case D3DCMP_NOTEQUAL: return BGFX_STATE_DEPTH_TEST_NOTEQUAL;
  case D3DCMP_GREATEREQUAL: return BGFX_STATE_DEPTH_TEST_GEQUAL;
  case D3DCMP_ALWAYS: return BGFX_STATE_DEPTH_TEST_ALWAYS;
  case D3DCMP_NEVER: return BGFX_STATE_DEPTH_TEST_NEVER;
  default: throw std::runtime_error("Unsupported game depth comparison");
  }
}
uint32_t StencilTest(DWORD value) {
  switch (value) {
  case D3DCMP_NEVER: return BGFX_STENCIL_TEST_NEVER;
  case D3DCMP_LESS: return BGFX_STENCIL_TEST_LESS;
  case D3DCMP_EQUAL: return BGFX_STENCIL_TEST_EQUAL;
  case D3DCMP_LESSEQUAL: return BGFX_STENCIL_TEST_LEQUAL;
  case D3DCMP_GREATER: return BGFX_STENCIL_TEST_GREATER;
  case D3DCMP_NOTEQUAL: return BGFX_STENCIL_TEST_NOTEQUAL;
  case D3DCMP_GREATEREQUAL: return BGFX_STENCIL_TEST_GEQUAL;
  case D3DCMP_ALWAYS: return BGFX_STENCIL_TEST_ALWAYS;
  default: throw std::runtime_error("Unsupported game stencil comparison");
  }
}
uint32_t StencilOp(DWORD value, int shift) {
  uint32_t operation;
  switch (value) {
  case D3DSTENCILOP_ZERO: operation = BGFX_STENCIL_OP_PASS_Z_ZERO; break;
  case D3DSTENCILOP_KEEP: operation = BGFX_STENCIL_OP_PASS_Z_KEEP; break;
  case D3DSTENCILOP_REPLACE: operation = BGFX_STENCIL_OP_PASS_Z_REPLACE; break;
  case D3DSTENCILOP_INCRSAT: operation = BGFX_STENCIL_OP_PASS_Z_INCRSAT; break;
  case D3DSTENCILOP_DECRSAT: operation = BGFX_STENCIL_OP_PASS_Z_DECRSAT; break;
  case D3DSTENCILOP_INVERT: operation = BGFX_STENCIL_OP_PASS_Z_INVERT; break;
  case D3DSTENCILOP_INCR: operation = BGFX_STENCIL_OP_PASS_Z_INCR; break;
  case D3DSTENCILOP_DECR: operation = BGFX_STENCIL_OP_PASS_Z_DECR; break;
  default: throw std::runtime_error("Unsupported game stencil operation");
  }
  return shift >= 0 ? operation << shift : operation >> -shift;
}
}

struct BgfxTextureStorage {
  unsigned width, height, levels, epoch;
  D3DFORMAT format;
  bool cube, system;
  bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
  std::vector<std::vector<unsigned char>> pixels;
  BgfxTextureStorage(unsigned w, unsigned h, unsigned l, D3DFORMAT f, bool c, bool s, bool target)
      : width(w), height(h), levels(l ? l : 1), epoch(activeEpoch), format(f), cube(c), system(s) {
    if (!l) { unsigned size = std::max(w,h); while (size > 1) { ++levels; size >>= 1; } }
    pixels.resize(levels * (cube ? 6 : 1));
    if (!system) {
      uint64_t flags = target ? BGFX_TEXTURE_RT : 0;
      if (!cube && !target) flags |= BGFX_TEXTURE_BLIT_DST;
      const auto native = TextureFormat(format);
      if (!bgfx::isTextureValid(0, cube, 1, native, flags)) throw std::runtime_error("Unsupported GPU texture format/usage");
      handle = cube ? bgfx::createTextureCube(w, levels > 1, 1, native, flags)
                    : bgfx::createTexture2D(w, h, levels > 1, 1, native, flags);
      if (!bgfx::isValid(handle)) throw std::runtime_error("bgfx texture allocation failed");
    }
  }
  ~BgfxTextureStorage() { if (epoch == activeEpoch && bgfx::isValid(handle)) bgfx::destroy(handle); }
  unsigned Width(unsigned level) const { return std::max(1u, width >> level); }
  unsigned Height(unsigned level) const { return std::max(1u, height >> level); }
  unsigned Pitch(unsigned level) const { unsigned block = BlockSize(format); return (Width(level)+block-1)/block * BytesPerBlock(format); }
  std::vector<unsigned char>& Pixels(unsigned level, unsigned face) {
    auto& data = pixels.at(face * levels + level);
    if (data.empty()) data.resize(Pitch(level) * ((Height(level)+BlockSize(format)-1)/BlockSize(format)));
    return data;
  }
  void Upload(unsigned level, unsigned face, const RECT& rect) {
    if (system) return;
    unsigned block = BlockSize(format), bytes = BytesPerBlock(format);
    unsigned w = rect.right - rect.left, h = rect.bottom - rect.top;
    unsigned pitch = (w+block-1)/block*bytes, rows = (h+block-1)/block;
    const bgfx::Memory* memory = bgfx::alloc(pitch*rows);
    auto& data = Pixels(level,face);
    for (unsigned y=0; y<rows; ++y) memcpy(memory->data+y*pitch,
        data.data()+(rect.top/block+y)*Pitch(level)+rect.left/block*bytes,pitch);
    if (cube) bgfx::updateTextureCube(handle,0,face,level,rect.left,rect.top,w,h,memory,pitch);
    else bgfx::updateTexture2D(handle,0,level,rect.left,rect.top,w,h,memory,pitch);
  }
};
BgfxSurface::BgfxSurface(std::shared_ptr<BgfxTextureStorage> s, unsigned l, unsigned f) : storage(std::move(s)), level(l), face(f) {}
HRESULT BgfxSurface::LockRect(D3DLOCKED_RECT* output,const RECT* rect,DWORD flags) {
  if (!output || level >= storage->levels || face >= (storage->cube ? 6u : 1u)) return E_INVALIDARG;
  lockedRect = rect ? *rect : RECT{0,0,LONG(storage->Width(level)),LONG(storage->Height(level))};
  if (lockedRect.left<0 || lockedRect.top<0 || lockedRect.right>LONG(storage->Width(level)) ||
      lockedRect.bottom>LONG(storage->Height(level)) || lockedRect.right<=lockedRect.left || lockedRect.bottom<=lockedRect.top) return E_INVALIDARG;
  readOnly = (flags & D3DLOCK_READONLY) != 0;
  unsigned block=BlockSize(storage->format);
  output->Pitch = storage->Pitch(level);
  output->pBits = storage->Pixels(level,face).data()+lockedRect.top/block*output->Pitch+lockedRect.left/block*BytesPerBlock(storage->format);
  return S_OK;
}
HRESULT BgfxSurface::UnlockRect() { if (!readOnly) storage->Upload(level,face,lockedRect); return S_OK; }
HRESULT BgfxSurface::GetDesc(D3DSURFACE_DESC* out) {
  if (!out) return E_INVALIDARG;
  *out = {}; out->Width=storage->Width(level);out->Height=storage->Height(level);out->Format=storage->format;return S_OK;
}
BgfxTexture::BgfxTexture(std::shared_ptr<BgfxTextureStorage> s) : storage(std::move(s)) {}
HRESULT BgfxTexture::GetSurfaceLevel(unsigned level,BgfxSurface** out) {
  if (!out || level>=storage->levels) return E_INVALIDARG;
  *out=new BgfxSurface(storage,level);return S_OK;
}
HRESULT BgfxTexture::GetCubeMapSurface(D3DCUBEMAP_FACES face,unsigned level,BgfxSurface** out) {
  if (!out || !storage->cube || unsigned(face)>=6 || level>=storage->levels) return E_INVALIDARG;
  *out=new BgfxSurface(storage,level,face);return S_OK;
}
BgfxBuffer::BgfxBuffer(unsigned size,bool wide) : bytes(size),index32(wide) {}
HRESULT BgfxBuffer::Lock(unsigned offset,unsigned size,void** out,DWORD) {
  if (!out || offset>bytes.size() || (size && size>bytes.size()-offset)) return E_INVALIDARG;
  *out=bytes.data()+offset; return S_OK;
}
HRESULT BgfxBuffer::Unlock() { return S_OK; }
BgfxDeclaration::BgfxDeclaration(const D3DVERTEXELEMENT9* elements) {
  layout.begin(bgfx::RendererType::Direct3D11);
  gpuLayout.begin(bgfx::RendererType::Direct3D11);
  unsigned offset=0;
  for (const D3DVERTEXELEMENT9* e=elements;e->Stream!=0xff;++e) {
    if (e->Stream!=0 || e->Offset<offset) throw std::runtime_error("Unsupported vertex stream");
    if (e->Offset>offset) layout.skip(e->Offset-offset);
    bgfx::Attrib::Enum attr=bgfx::Attrib::Count;
    switch (e->Usage) {
    case D3DDECLUSAGE_POSITION: attr=bgfx::Attrib::Position;break;
    case D3DDECLUSAGE_NORMAL: attr=bgfx::Attrib::Normal;break;
    case D3DDECLUSAGE_COLOR: attr=e->UsageIndex ? bgfx::Attrib::Color1 : bgfx::Attrib::Color0;break;
    case D3DDECLUSAGE_TEXCOORD: attr=bgfx::Attrib::Enum(bgfx::Attrib::TexCoord0+e->UsageIndex);break;
    case D3DDECLUSAGE_TANGENT: attr=e->UsageIndex ? bgfx::Attrib::Bitangent : bgfx::Attrib::Tangent;break;
    }
    if (attr==bgfx::Attrib::Count) throw std::runtime_error("Unsupported vertex attribute");
    unsigned components=e->Type==D3DDECLTYPE_FLOAT3 ? 3 : e->Type==D3DDECLTYPE_D3DCOLOR ? 4 : 2;
    gpuLayout.add(attr,components,bgfx::AttribType::Float);
    switch(e->Type) {
    case D3DDECLTYPE_FLOAT3:layout.add(attr,3,bgfx::AttribType::Float);offset=e->Offset+12;break;
    case D3DDECLTYPE_FLOAT2:layout.add(attr,2,bgfx::AttribType::Float);offset=e->Offset+8;break;
    case D3DDECLTYPE_D3DCOLOR:layout.add(attr,4,bgfx::AttribType::Uint8,true);offset=e->Offset+4;break;
    case D3DDECLTYPE_SHORT2:layout.add(attr,2,bgfx::AttribType::Int16,false);offset=e->Offset+4;break;
    default:throw std::runtime_error("Unsupported vertex encoding");
    }
  }
  layout.end();
  gpuLayout.end();
}
void BgfxDeclaration::CopyVertices(void* output,const void* input,unsigned count) const {
  memset(output,0,count*gpuLayout.getStride());
  for(unsigned vertex=0;vertex<count;++vertex) for(unsigned a=0;a<bgfx::Attrib::Count;++a) {
    auto attr=bgfx::Attrib::Enum(a);
    if(!layout.has(attr))continue;
    float value[4]={0,0,0,1};bgfx::vertexUnpack(value,attr,layout,input,vertex);
    uint8_t components;bgfx::AttribType::Enum type;bool normalized,asInteger;
    layout.decode(attr,components,type,normalized,asInteger);
    if(type==bgfx::AttribType::Int16) {
      const auto* packed=reinterpret_cast<const int16_t*>(static_cast<const unsigned char*>(input)+vertex*layout.getStride()+layout.getOffset(attr));
      for(unsigned component=0;component<components;++component)value[component]=float(packed[component]);
    }
    if(type==bgfx::AttribType::Uint8)std::swap(value[0],value[2]);
    bgfx::vertexPack(value,false,attr,gpuLayout,output,vertex);
  }
}

struct BgfxDevice::Impl {
  bool initialized=false,healthy=true;
  unsigned width=0,height=0;
  bgfx::ViewId nextView=0,currentView=0;
  bool viewDirty=true,viewUsed=false;
  NWin32Helper::com_ptr<BgfxSurface> screenColor,screenDepth,color,depth;
  NWin32Helper::com_ptr<BgfxBuffer> vertices,indices;
  NWin32Helper::com_ptr<BgfxDeclaration> declaration;
  NWin32Helper::com_ptr<BgfxShader> vertexShader,pixelShader;
  std::array<NWin32Helper::com_ptr<BgfxTexture>,8> textures;
  unsigned vertexOffset=0,stride=0;
  float vertexConstants[96][4]{},pixelConstants[8][4]{},projection[8]{};
  std::array<DWORD,256> states{};
  DWORD sampler[8][16]{};
  std::map<uint64_t,bgfx::ShaderHandle> shaders;
  std::map<uint64_t,bgfx::ProgramHandle> programs;
  std::vector<bgfx::FrameBufferHandle> frameBuffers;
  std::vector<NWin32Helper::com_ptr<BgfxSurface>> frameSurfaces;
  bgfx::UniformHandle uVertex=BGFX_INVALID_HANDLE,uPixel=BGFX_INVALID_HANDLE,uAlpha=BGFX_INVALID_HANDLE;
  bgfx::UniformHandle uProjection=BGFX_INVALID_HANDLE,uPresent=BGFX_INVALID_HANDLE;
  bgfx::UniformHandle uRaster=BGFX_INVALID_HANDLE;
  bgfx::UniformHandle samplers[8];
  bgfx::ProgramHandle presentProgram=BGFX_INVALID_HANDLE;
  Impl() {
    states[D3DRS_ZENABLE]=true;states[D3DRS_ZWRITEENABLE]=true;states[D3DRS_ZFUNC]=D3DCMP_LESSEQUAL;
    states[D3DRS_COLORWRITEENABLE]=15;states[D3DRS_CULLMODE]=D3DCULL_CCW;
    states[D3DRS_SRCBLEND]=D3DBLEND_ONE;states[D3DRS_DESTBLEND]=D3DBLEND_ZERO;
    states[D3DRS_STENCILFUNC]=D3DCMP_ALWAYS;
    states[D3DRS_STENCILPASS]=states[D3DRS_STENCILFAIL]=states[D3DRS_STENCILZFAIL]=D3DSTENCILOP_KEEP;
    states[D3DRS_STENCILMASK]=states[D3DRS_STENCILWRITEMASK]=255;
    for(auto& stage:sampler) { stage[D3DSAMP_ADDRESSU]=stage[D3DSAMP_ADDRESSV]=D3DTADDRESS_CLAMP;
      stage[D3DSAMP_MAGFILTER]=stage[D3DSAMP_MINFILTER]=stage[D3DSAMP_MIPFILTER]=D3DTEXF_LINEAR; }
  }
  HRESULT Fail(const char* message) {
    healthy=false;fprintf(stderr,"bgfx game renderer: %s\n",message);OutputDebugStringA(message);return E_FAIL;
  }
  bgfx::ShaderHandle Shader(int id,bool vertex,int mask=0) {
    unsigned available=0;
    for(const auto& binary:S2GameShaders::all) if(binary.id==id && binary.vertex==vertex) available|=binary.cubeMask;
    mask&=available;
    uint64_t key=(uint64_t(vertex)<<32)|(unsigned(id)<<8)|unsigned(mask);
    auto found=shaders.find(key);if(found!=shaders.end()) return found->second;
    for(const auto& binary:S2GameShaders::all) if(binary.id==id && binary.vertex==vertex && binary.cubeMask==mask) {
      auto handle=bgfx::createShader(bgfx::copy(binary.data,binary.size));
      if(!bgfx::isValid(handle)) throw std::runtime_error("bgfx rejected game shader");
      shaders[key]=handle;return handle;
    }
    throw std::runtime_error("Missing compiled game shader variant");
  }
  bgfx::ProgramHandle Program() {
    if(!vertexShader || !pixelShader) throw std::runtime_error("Draw without game shaders");
    unsigned mask=0;
    for(unsigned i=0;i<8;++i) if(textures[i] && textures[i]->storage->cube) mask|=1u<<i;
    uint64_t key=(uint64_t(vertexShader->id)<<32)|(unsigned(pixelShader->id)<<8)|mask;
    auto found=programs.find(key);if(found!=programs.end()) return found->second;
    auto handle=bgfx::createProgram(Shader(vertexShader->id,true),Shader(pixelShader->id,false,mask),false);
    if(!bgfx::isValid(handle)) throw std::runtime_error("bgfx game shader link failed");
    programs[key]=handle;return handle;
  }
  bgfx::ViewId View() {
    if(!viewDirty) return currentView;
    if(nextView>=2045) throw std::runtime_error("Game exceeded ordered render-pass limit");
    if(!color || !depth) throw std::runtime_error("Render pass without color/depth surface");
    currentView=nextView++;
    const auto& c=*color->storage;const auto& d=*depth->storage;
    bgfx::Attachment attachments[2];
    attachments[0].init(c.handle,bgfx::Access::Write,color->face,1,color->level,0);
    attachments[1].init(d.handle,bgfx::Access::Write,depth->face,1,depth->level,0);
    auto frame=bgfx::createFrameBuffer(2,attachments,false);
    if(!bgfx::isValid(frame)) throw std::runtime_error("bgfx render target creation failed");
    frameBuffers.push_back(frame);frameSurfaces.push_back(color);frameSurfaces.push_back(depth);
    bgfx::setViewFrameBuffer(currentView,frame);
    bgfx::setViewRect(currentView,0,0,c.Width(color->level),c.Height(color->level));
    bgfx::setViewMode(currentView,bgfx::ViewMode::Sequential);
    bgfx::setViewClear(currentView,BGFX_CLEAR_NONE);
    viewDirty=false;viewUsed=false;
    return currentView;
  }
  void EndFrame() {
    for(auto frame:frameBuffers) bgfx::destroy(frame);
    frameBuffers.clear();frameSurfaces.clear();nextView=0;viewDirty=true;viewUsed=false;
  }
  void Apply(D3DPRIMITIVETYPE primitive) {
    // Keep the legacy integer pixel centers on the D3D11 half-integer grid.
    float raster[]={1.0f/color->storage->Width(color->level),-1.0f/color->storage->Height(color->level),0,0};
    bgfx::setUniform(uRaster,raster);
    bgfx::setUniform(uVertex,vertexConstants,96);bgfx::setUniform(uPixel,pixelConstants,8);
    float alpha[4]={states[D3DRS_ALPHAREF]/255.0f,float(states[D3DRS_ALPHATESTENABLE]),float(states[D3DRS_ALPHAFUNC]),0};
    bgfx::setUniform(uAlpha,alpha);bgfx::setUniform(uProjection,projection,2);
    for(unsigned i=0;i<8;++i) if(textures[i]) {
      uint32_t flags=0;
      if(sampler[i][D3DSAMP_ADDRESSU]==D3DTADDRESS_CLAMP)flags|=BGFX_SAMPLER_U_CLAMP;
      if(sampler[i][D3DSAMP_ADDRESSV]==D3DTADDRESS_CLAMP)flags|=BGFX_SAMPLER_V_CLAMP;
      if(sampler[i][D3DSAMP_MINFILTER]==D3DTEXF_POINT)flags|=BGFX_SAMPLER_MIN_POINT;
      if(sampler[i][D3DSAMP_MAGFILTER]==D3DTEXF_POINT)flags|=BGFX_SAMPLER_MAG_POINT;
      if(sampler[i][D3DSAMP_MINFILTER]==D3DTEXF_ANISOTROPIC)flags|=BGFX_SAMPLER_MIN_ANISOTROPIC;
      if(sampler[i][D3DSAMP_MAGFILTER]==D3DTEXF_ANISOTROPIC)flags|=BGFX_SAMPLER_MAG_ANISOTROPIC;
      if(sampler[i][D3DSAMP_MIPFILTER]!=D3DTEXF_LINEAR)flags|=BGFX_SAMPLER_MIP_POINT;
      bgfx::setTexture(i,samplers[i],textures[i]->storage->handle,flags);
    }
    uint64_t state=0;DWORD write=states[D3DRS_COLORWRITEENABLE];
    if(write&1)state|=BGFX_STATE_WRITE_R;if(write&2)state|=BGFX_STATE_WRITE_G;
    if(write&4)state|=BGFX_STATE_WRITE_B;if(write&8)state|=BGFX_STATE_WRITE_A;
    if(states[D3DRS_ZENABLE])state|=Depth(states[D3DRS_ZFUNC]);
    if(states[D3DRS_ZWRITEENABLE])state|=BGFX_STATE_WRITE_Z;
    if(states[D3DRS_CULLMODE]==D3DCULL_CW)state|=BGFX_STATE_CULL_CW;
    if(states[D3DRS_CULLMODE]==D3DCULL_CCW)state|=BGFX_STATE_CULL_CCW;
    if(states[D3DRS_ALPHABLENDENABLE])state|=BGFX_STATE_BLEND_FUNC(Blend(states[D3DRS_SRCBLEND]),Blend(states[D3DRS_DESTBLEND]));
    if(primitive==D3DPT_LINELIST)state|=BGFX_STATE_PT_LINES;
    else if(primitive==D3DPT_LINESTRIP)state|=BGFX_STATE_PT_LINESTRIP;
    else if(primitive!=D3DPT_TRIANGLELIST)throw std::runtime_error("Unsupported game primitive");
    bgfx::setState(state);
    uint32_t stencil=BGFX_STENCIL_NONE;
    if(states[D3DRS_STENCILENABLE]) {
      stencil=StencilTest(states[D3DRS_STENCILFUNC])|BGFX_STENCIL_FUNC_REF(states[D3DRS_STENCILREF])|
              BGFX_STENCIL_FUNC_RMASK(states[D3DRS_STENCILMASK])|
              StencilOp(states[D3DRS_STENCILFAIL],-8)|
              StencilOp(states[D3DRS_STENCILZFAIL],-4)|
              StencilOp(states[D3DRS_STENCILPASS],0);
    }
    // The pinned bgfx packs the write mask in the back state read-mask field.
    bgfx::setStencil(stencil,states[D3DRS_STENCILENABLE] ? BGFX_STENCIL_FUNC_RMASK(states[D3DRS_STENCILWRITEMASK]) : BGFX_STENCIL_NONE);
  }
};
BgfxDevice::BgfxDevice() : impl(new Impl) {}
bool BgfxDevice::Init(HWND window,unsigned w,unsigned h) {
  if(!window || !w || !h || activeEpoch) return false;
  bgfx::Init init;
  init.type=bgfx::RendererType::Direct3D11;init.swapChain.nwh=window;
  init.swapChain.width=w;init.swapChain.height=h;init.reset=BGFX_RESET_NONE;
  init.limits.maxTransientVbSize=64*1024*1024;init.limits.maxTransientIbSize=16*1024*1024;
  if(!bgfx::init(init)) { impl->Fail("Cannot initialize bgfx Direct3D11");return false; }
  activeEpoch=++nextEpoch;impl->initialized=true;
  try {
    impl->uVertex=bgfx::createUniform("u_vertexRegisters",bgfx::UniformType::Vec4,96);
    impl->uPixel=bgfx::createUniform("u_pixelRegisters",bgfx::UniformType::Vec4,8);
    impl->uAlpha=bgfx::createUniform("u_alphaTest",bgfx::UniformType::Vec4);
    impl->uProjection=bgfx::createUniform("u_projectionFlags",bgfx::UniformType::Vec4,2);
    impl->uPresent=bgfx::createUniform("u_present",bgfx::UniformType::Vec4);
    impl->uRaster=bgfx::createUniform("u_rasterOffset",bgfx::UniformType::Vec4);
    for(unsigned i=0;i<8;++i) {char name[16];sprintf_s(name,"s%u",i);impl->samplers[i]=bgfx::createUniform(name,bgfx::UniformType::Sampler);}
    impl->presentProgram=bgfx::createProgram(impl->Shader(0,true),impl->Shader(0,false),false);
    if(!bgfx::isValid(impl->presentProgram))throw std::runtime_error("Cannot link presentation shaders");
    fprintf(stderr,"Game renderer: bgfx %s, GPU %04x:%04x\n",bgfx::getRendererName(bgfx::getRendererType()),bgfx::getCaps()->vendorId,bgfx::getCaps()->deviceId);
    return Resize(w,h);
  }catch(const std::exception& e){impl->Fail(e.what());return false;}
}
BgfxDevice::~BgfxDevice() {
  if(!impl->initialized)return;
  impl->EndFrame();
  impl->vertices=0;impl->indices=0;impl->declaration=0;impl->vertexShader=0;impl->pixelShader=0;
  for(auto& t:impl->textures)t=0;
  impl->color=0;impl->depth=0;impl->screenColor=0;impl->screenDepth=0;
  for(auto p:impl->programs)bgfx::destroy(p.second);
  if(bgfx::isValid(impl->presentProgram))bgfx::destroy(impl->presentProgram);
  for(auto s:impl->shaders)bgfx::destroy(s.second);
  for(auto uniform:{impl->uVertex,impl->uPixel,impl->uAlpha,impl->uProjection,impl->uPresent,impl->uRaster})if(bgfx::isValid(uniform))bgfx::destroy(uniform);
  for(auto s:impl->samplers)if(bgfx::isValid(s))bgfx::destroy(s);
  bgfx::shutdown();activeEpoch=0;
}
bool BgfxDevice::Resize(unsigned w,unsigned h) {
  if(!impl->initialized || !w || !h || w>16384 || h>16384)return false;
  try {
    impl->EndFrame();bgfx::SwapChain swapChain;swapChain.width=w;swapChain.height=h;
    bgfx::reset(BGFX_RESET_NONE,&swapChain);
    impl->width=w;impl->height=h;
    impl->screenColor.Create(new BgfxSurface(std::make_shared<BgfxTextureStorage>(w,h,1,D3DFMT_A8R8G8B8,false,false,true)));
    impl->screenDepth.Create(new BgfxSurface(std::make_shared<BgfxTextureStorage>(w,h,1,D3DFMT_D24S8,false,false,true)));
    impl->color=impl->screenColor;impl->depth=impl->screenDepth;
    return true;
  }catch(const std::exception& e){impl->Fail(e.what());return false;}
}
bool BgfxDevice::Healthy() const {return impl->initialized && impl->healthy;}
void BgfxDevice::Present(float gamma) {
  if(!Healthy())return;
  struct Vertex {float x,y,z,u,v;};
  Vertex vertices[]={{-1,1,0,0,0},{3,1,0,2,0},{-1,-3,0,0,2}};
  bgfx::VertexLayout layout;layout.begin().add(bgfx::Attrib::Position,3,bgfx::AttribType::Float).add(bgfx::Attrib::TexCoord0,2,bgfx::AttribType::Float).end();
  if(bgfx::getAvailTransientVertexBuffer(3,layout)<3){impl->Fail("Presentation geometry budget exhausted");return;}
  bgfx::TransientVertexBuffer buffer;bgfx::allocTransientVertexBuffer(&buffer,3,layout);memcpy(buffer.data,vertices,sizeof(vertices));
  bgfx::setViewFrameBuffer(2047,BGFX_INVALID_HANDLE);bgfx::setViewRect(2047,0,0,impl->width,impl->height);
  bgfx::setViewMode(2047,bgfx::ViewMode::Sequential);bgfx::setViewClear(2047,BGFX_CLEAR_NONE);
  bgfx::setVertexBuffer(0,&buffer);bgfx::setTexture(0,impl->samplers[0],impl->screenColor->storage->handle,BGFX_SAMPLER_U_CLAMP|BGFX_SAMPLER_V_CLAMP|BGFX_SAMPLER_MIN_POINT|BGFX_SAMPLER_MAG_POINT);
  float params[]={1.0f/std::max(0.1f,gamma),0,0,0};bgfx::setUniform(impl->uPresent,params);
  bgfx::setState(BGFX_STATE_WRITE_RGB|BGFX_STATE_WRITE_A);bgfx::setStencil(BGFX_STENCIL_NONE);
  bgfx::submit(2047,impl->presentProgram);bgfx::frame();impl->EndFrame();
}
void BgfxDevice::Screenshot(std::vector<unsigned char>* pixels,unsigned* width,unsigned* height) {
  if(!Healthy())return;
  *width=impl->width;*height=impl->height;pixels->resize(*width**height*4);
  auto readback=bgfx::createTexture2D(*width,*height,false,1,bgfx::TextureFormat::BGRA8,BGFX_TEXTURE_BLIT_DST|BGFX_TEXTURE_READ_BACK);
  bgfx::TextureRegion destination,source;
  destination.init(readback);source.init(impl->screenColor->storage->handle);
  bgfx::blit(2046,destination,source);
  uint32_t ready=bgfx::read(destination,pixels->data());
  uint32_t frame=bgfx::frame();impl->EndFrame();while(frame<ready)frame=bgfx::frame();
  bgfx::destroy(readback);
}
HRESULT BgfxDevice::CreateTexture(unsigned w,unsigned h,unsigned levels,DWORD usage,D3DFORMAT format,D3DPOOL pool,BgfxTexture** out,HANDLE*) {
  if(!out || !w || !h || w>16384 || h>16384)return E_INVALIDARG;
  *out=nullptr;
  try { *out=new BgfxTexture(std::make_shared<BgfxTextureStorage>(w,h,levels,format,false,pool==D3DPOOL_SYSTEMMEM,(usage&D3DUSAGE_RENDERTARGET)!=0));return S_OK; }
  catch(const std::exception& e){return impl->Fail(e.what());}
}
HRESULT BgfxDevice::CreateCubeTexture(unsigned size,unsigned levels,DWORD usage,D3DFORMAT format,D3DPOOL pool,BgfxTexture** out,HANDLE*) {
  if(!out || !size || size>16384)return E_INVALIDARG;*out=nullptr;
  try{*out=new BgfxTexture(std::make_shared<BgfxTextureStorage>(size,size,levels,format,true,pool==D3DPOOL_SYSTEMMEM,(usage&D3DUSAGE_RENDERTARGET)!=0));return S_OK;}
  catch(const std::exception& e){return impl->Fail(e.what());}
}
HRESULT BgfxDevice::CreateDepthStencilSurface(unsigned w,unsigned h,D3DFORMAT format,D3DMULTISAMPLE_TYPE,DWORD,BOOL,BgfxSurface** out,HANDLE*) {
  if(!out || !w || !h)return E_INVALIDARG;*out=nullptr;
  try{*out=new BgfxSurface(std::make_shared<BgfxTextureStorage>(w,h,1,format,false,false,true));return S_OK;}
  catch(const std::exception& e){return impl->Fail(e.what());}
}
HRESULT BgfxDevice::CreateVertexBuffer(unsigned size,DWORD,DWORD,D3DPOOL,BgfxBuffer** out,HANDLE*) {if(!out || !size)return E_INVALIDARG;*out=new BgfxBuffer(size);return S_OK;}
HRESULT BgfxDevice::CreateIndexBuffer(unsigned size,DWORD,D3DFORMAT format,D3DPOOL,BgfxBuffer** out,HANDLE*) {if(!out || !size)return E_INVALIDARG;*out=new BgfxBuffer(size,format==D3DFMT_INDEX32);return S_OK;}
HRESULT BgfxDevice::CreateVertexDeclaration(const D3DVERTEXELEMENT9* elements,BgfxDeclaration** out) {
  if(!out || !elements)return E_INVALIDARG;*out=nullptr;
  try{*out=new BgfxDeclaration(elements);return S_OK;}catch(const std::exception& e){return impl->Fail(e.what());}
}
HRESULT BgfxDevice::CreateVertexShader(const DWORD* tokens,BgfxShader** out) {
  if(!out)return E_INVALIDARG;*out=nullptr;
  for(auto shader:vsAllShaders)if(shader->pShader==tokens){
    try{impl->Shader(shader->nID,true);*out=new BgfxShader(shader->nID,true);return S_OK;}
    catch(const std::exception& e){return impl->Fail(e.what());}
  }
  return impl->Fail("Unregistered vertex shader");
}
HRESULT BgfxDevice::CreatePixelShader(const DWORD* tokens,BgfxShader** out) {
  if(!out)return E_INVALIDARG;*out=nullptr;
  for(auto shader:psAllShaders)if(shader->pShader14==tokens || shader->pShader==tokens){
    try{impl->Shader(shader->nID,false);*out=new BgfxShader(shader->nID,false);return S_OK;}
    catch(const std::exception& e){return impl->Fail(e.what());}
  }
  return impl->Fail("Unregistered pixel shader");
}
HRESULT BgfxDevice::SetStreamSource(unsigned stream,BgfxBuffer* buffer,unsigned offset,unsigned stride) {if(stream)return E_INVALIDARG;impl->vertices=buffer;impl->vertexOffset=offset;impl->stride=stride;return S_OK;}
HRESULT BgfxDevice::SetIndices(BgfxBuffer* buffer){impl->indices=buffer;return S_OK;}
HRESULT BgfxDevice::SetTexture(unsigned stage,BgfxTexture* texture){if(stage>=8)return E_INVALIDARG;impl->textures[stage]=texture;return S_OK;}
HRESULT BgfxDevice::SetVertexDeclaration(BgfxDeclaration* declaration){impl->declaration=declaration;return S_OK;}
HRESULT BgfxDevice::SetVertexShader(BgfxShader* shader){impl->vertexShader=shader;return S_OK;}
HRESULT BgfxDevice::SetPixelShader(BgfxShader* shader){impl->pixelShader=shader;return S_OK;}
HRESULT BgfxDevice::SetVertexShaderConstantF(unsigned start,const float* data,unsigned count){if(!data || start>96 || count>96-start)return E_INVALIDARG;memcpy(impl->vertexConstants[start],data,count*16);return S_OK;}
HRESULT BgfxDevice::SetPixelShaderConstantF(unsigned start,const float* data,unsigned count){if(!data || start>8 || count>8-start)return E_INVALIDARG;memcpy(impl->pixelConstants[start],data,count*16);return S_OK;}
HRESULT BgfxDevice::SetRenderState(D3DRENDERSTATETYPE state,DWORD value){if(unsigned(state)>=impl->states.size())return E_INVALIDARG;impl->states[state]=value;if(state==D3DRS_FILLMODE)bgfx::setDebug(value==D3DFILL_WIREFRAME?BGFX_DEBUG_WIREFRAME:BGFX_DEBUG_NONE);return S_OK;}
HRESULT BgfxDevice::SetSamplerState(unsigned stage,D3DSAMPLERSTATETYPE state,DWORD value){if(stage>=8 || unsigned(state)>=16)return E_INVALIDARG;impl->sampler[stage][state]=value;return S_OK;}
HRESULT BgfxDevice::SetTextureStageState(unsigned stage,D3DTEXTURESTAGESTATETYPE state,DWORD value){if(stage>=8)return E_INVALIDARG;if(state==D3DTSS_TEXTURETRANSFORMFLAGS)impl->projection[stage]=(value&D3DTTFF_PROJECTED)?1.0f:0.0f;return S_OK;}
HRESULT BgfxDevice::SetRenderTarget(unsigned slot,BgfxSurface* surface){if(slot || !surface)return E_INVALIDARG;if(impl->color!=surface){impl->color=surface;impl->viewDirty=true;}return S_OK;}
HRESULT BgfxDevice::SetDepthStencilSurface(BgfxSurface* surface){if(!surface)return E_INVALIDARG;if(impl->depth!=surface){impl->depth=surface;impl->viewDirty=true;}return S_OK;}
HRESULT BgfxDevice::GetRenderTarget(unsigned slot,BgfxSurface** out){if(slot || !out)return E_INVALIDARG;*out=impl->screenColor;if(*out)(*out)->AddRef();return S_OK;}
HRESULT BgfxDevice::GetDepthStencilSurface(BgfxSurface** out){if(!out)return E_INVALIDARG;*out=impl->screenDepth;if(*out)(*out)->AddRef();return S_OK;}
HRESULT BgfxDevice::Clear(DWORD count,const D3DRECT*,DWORD flags,D3DCOLOR color,float z,DWORD stencil) {
  if(count)return E_NOTIMPL;
  try{
    if(impl->viewUsed)impl->viewDirty=true;
    auto view=impl->View();uint16_t clear=0;
    if(flags&D3DCLEAR_TARGET)clear|=BGFX_CLEAR_COLOR;if(flags&D3DCLEAR_ZBUFFER)clear|=BGFX_CLEAR_DEPTH;if(flags&D3DCLEAR_STENCIL)clear|=BGFX_CLEAR_STENCIL;
    bgfx::setViewClear(view,clear,(color<<8)|(color>>24),z,stencil);bgfx::touch(view);impl->viewUsed=true;return S_OK;
  }catch(const std::exception& e){return impl->Fail(e.what());}
}
HRESULT BgfxDevice::DrawIndexedPrimitive(D3DPRIMITIVETYPE primitive,int base,unsigned,unsigned,unsigned start,unsigned count) {
  if(!count)return S_OK;
  try{
    if(!impl->vertices || !impl->indices || !impl->declaration)throw std::runtime_error("Draw with missing vertex/index data");
    unsigned num=primitive==D3DPT_TRIANGLELIST?count*3:primitive==D3DPT_LINELIST?count*2:count+1;
    unsigned indexSize=impl->indices->index32?4:2;
    if(uint64_t(start+uint64_t(num))*indexSize>impl->indices->bytes.size())throw std::runtime_error("Game index buffer bounds violation");
    std::vector<uint32_t> source(num);uint32_t low=UINT32_MAX,high=0;
    for(unsigned i=0;i<num;++i){uint32_t index=0;memcpy(&index,impl->indices->bytes.data()+uint64_t(start+i)*indexSize,indexSize);int64_t effective=int64_t(index)+base;
      if(effective<0 || effective>UINT32_MAX)throw std::runtime_error("Invalid game base vertex");
      source[i]=uint32_t(effective);low=std::min(low,source[i]);high=std::max(high,source[i]);}
    unsigned vertices=high-low+1;const auto& layout=impl->declaration->gpuLayout;
    if(impl->declaration->layout.getStride()!=impl->stride || uint64_t(high+uint64_t(1))*impl->stride+impl->vertexOffset>impl->vertices->bytes.size())throw std::runtime_error("Game vertex buffer bounds violation");
    bool wide=vertices>65536;
    if(bgfx::getAvailTransientVertexBuffer(vertices,layout)<vertices || bgfx::getAvailTransientIndexBuffer(num,wide)<num)throw std::runtime_error("Transient game geometry budget exhausted");
    bgfx::TransientVertexBuffer vb;bgfx::TransientIndexBuffer ib;bgfx::allocTransientVertexBuffer(&vb,vertices,layout);bgfx::allocTransientIndexBuffer(&ib,num,wide);
    impl->declaration->CopyVertices(vb.data,impl->vertices->bytes.data()+impl->vertexOffset+uint64_t(low)*impl->stride,vertices);
    for(unsigned i=0;i<num;++i){uint32_t value=source[i]-low;memcpy(ib.data+i*(wide?4:2),&value,wide?4:2);}
    bgfx::setVertexBuffer(0,&vb);bgfx::setIndexBuffer(&ib);impl->Apply(primitive);
    auto program=impl->Program();auto view=impl->View();bgfx::submit(view,program);impl->viewUsed=true;return S_OK;
  }catch(const std::exception& e){return impl->Fail(e.what());}
}
HRESULT BgfxDevice::DrawPrimitive(D3DPRIMITIVETYPE primitive,unsigned start,unsigned count) {
  if(!count)return S_OK;
  try{
    unsigned num=primitive==D3DPT_LINESTRIP?count+1:primitive==D3DPT_LINELIST?count*2:count*3;
    if(!impl->vertices || !impl->declaration || uint64_t(start+uint64_t(num))*impl->stride+impl->vertexOffset>impl->vertices->bytes.size())throw std::runtime_error("Invalid game nonindexed draw");
    const auto& layout=impl->declaration->gpuLayout;
    if(bgfx::getAvailTransientVertexBuffer(num,layout)<num)throw std::runtime_error("Transient geometry budget exhausted");
    bgfx::TransientVertexBuffer vb;bgfx::allocTransientVertexBuffer(&vb,num,layout);impl->declaration->CopyVertices(vb.data,impl->vertices->bytes.data()+impl->vertexOffset+uint64_t(start)*impl->stride,num);
    bgfx::setVertexBuffer(0,&vb);impl->Apply(primitive);auto program=impl->Program();auto view=impl->View();bgfx::submit(view,program);impl->viewUsed=true;return S_OK;
  }catch(const std::exception& e){return impl->Fail(e.what());}
}
HRESULT BgfxDevice::UpdateSurface(BgfxSurface* source,const RECT* rect,BgfxSurface* destination,const POINT* point) {
  if(!source || !destination || source->storage->format!=destination->storage->format)return E_INVALIDARG;
  RECT area=rect?*rect:RECT{0,0,LONG(source->storage->Width(source->level)),LONG(source->storage->Height(source->level))};
  POINT at=point?*point:POINT{};RECT target={at.x,at.y,at.x+area.right-area.left,at.y+area.bottom-area.top};
  if(area.left<0 || area.top<0 || area.right>LONG(source->storage->Width(source->level)) || area.bottom>LONG(source->storage->Height(source->level)) || target.left<0 || target.top<0 || target.right>LONG(destination->storage->Width(destination->level)) || target.bottom>LONG(destination->storage->Height(destination->level)))return E_INVALIDARG;
  unsigned block=BlockSize(source->storage->format),bytes=BytesPerBlock(source->storage->format);
  auto& src=source->storage->Pixels(source->level,source->face);auto& dst=destination->storage->Pixels(destination->level,destination->face);
  unsigned rows=(area.bottom-area.top+block-1)/block,span=(area.right-area.left+block-1)/block*bytes;
  for(unsigned y=0;y<rows;++y)memcpy(dst.data()+(target.top/block+y)*destination->storage->Pitch(destination->level)+target.left/block*bytes,
      src.data()+(area.top/block+y)*source->storage->Pitch(source->level)+area.left/block*bytes,span);
  destination->storage->Upload(destination->level,destination->face,target);return S_OK;
}
HRESULT BgfxDevice::BeginScene(){return Healthy()?S_OK:E_FAIL;}
HRESULT BgfxDevice::EndScene(){return Healthy()?S_OK:E_FAIL;}
HRESULT BgfxDevice::ValidateDevice(DWORD* passes){if(!passes)return E_INVALIDARG;*passes=1;return Healthy()?S_OK:E_FAIL;}
}
