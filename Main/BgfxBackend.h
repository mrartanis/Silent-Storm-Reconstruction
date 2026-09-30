#pragma once
#include <d3d9.h> // Historical data/state tokens only; no D3D9 device or runtime.
#include <bgfx/bgfx.h>
#include <atomic>
#include <memory>
#include <vector>

namespace NGfx {
class BgfxResource {
  std::atomic<unsigned> references{1};
public:
  virtual ~BgfxResource() = default;
  unsigned AddRef() { return ++references; }
  unsigned Release() { unsigned left = --references; if (!left) delete this; return left; }
};
struct BgfxTextureStorage;
class BgfxSurface : public BgfxResource {
public:
  std::shared_ptr<BgfxTextureStorage> storage;
  unsigned level = 0, face = 0;
  RECT lockedRect{};
  bool readOnly = false;
  BgfxSurface(std::shared_ptr<BgfxTextureStorage> s, unsigned l = 0, unsigned f = 0);
  HRESULT LockRect(D3DLOCKED_RECT*, const RECT*, DWORD);
  HRESULT UnlockRect();
  HRESULT GetDesc(D3DSURFACE_DESC*);
};
class BgfxTexture : public BgfxResource {
public:
  std::shared_ptr<BgfxTextureStorage> storage;
  explicit BgfxTexture(std::shared_ptr<BgfxTextureStorage> s);
  HRESULT GetSurfaceLevel(unsigned, BgfxSurface**);
  HRESULT GetCubeMapSurface(D3DCUBEMAP_FACES, unsigned, BgfxSurface**);
};
class BgfxBuffer : public BgfxResource {
public:
  std::vector<unsigned char> bytes;
  bool index32 = false;
  explicit BgfxBuffer(unsigned size, bool wide = false);
  HRESULT Lock(unsigned offset, unsigned size, void** output, DWORD flags);
  HRESULT Unlock();
};
class BgfxShader : public BgfxResource {
public:
  int id;
  bool vertex;
  BgfxShader(int i, bool v) : id(i), vertex(v) {}
};
class BgfxDeclaration : public BgfxResource {
public:
  bgfx::VertexLayout layout;
  bgfx::VertexLayout gpuLayout;
  explicit BgfxDeclaration(const D3DVERTEXELEMENT9*);
  void CopyVertices(void* output,const void* input,unsigned count) const;
};
class BgfxDevice : public BgfxResource {
  struct Impl;
  std::unique_ptr<Impl> impl;
public:
  BgfxDevice();
  ~BgfxDevice();
  bool Init(HWND, unsigned width, unsigned height);
  bool Resize(unsigned width, unsigned height);
  bool Healthy() const;
  void Present(float gamma);
  void Screenshot(std::vector<unsigned char>*, unsigned*, unsigned*);
  HRESULT CreateTexture(unsigned, unsigned, unsigned, DWORD, D3DFORMAT, D3DPOOL, BgfxTexture**, HANDLE*);
  HRESULT CreateCubeTexture(unsigned, unsigned, DWORD, D3DFORMAT, D3DPOOL, BgfxTexture**, HANDLE*);
  HRESULT CreateDepthStencilSurface(unsigned, unsigned, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL, BgfxSurface**, HANDLE*);
  HRESULT CreateVertexBuffer(unsigned size, DWORD, DWORD, D3DPOOL, BgfxBuffer**, HANDLE*);
  HRESULT CreateIndexBuffer(unsigned size, DWORD, D3DFORMAT, D3DPOOL, BgfxBuffer**, HANDLE*);
  HRESULT CreateVertexDeclaration(const D3DVERTEXELEMENT9*, BgfxDeclaration**);
  HRESULT CreateVertexShader(const DWORD*, BgfxShader**);
  HRESULT CreatePixelShader(const DWORD*, BgfxShader**);
  HRESULT SetStreamSource(unsigned, BgfxBuffer*, unsigned offset, unsigned stride);
  HRESULT SetIndices(BgfxBuffer*);
  HRESULT SetTexture(unsigned, BgfxTexture*);
  HRESULT SetVertexDeclaration(BgfxDeclaration*);
  HRESULT SetVertexShader(BgfxShader*);
  HRESULT SetPixelShader(BgfxShader*);
  HRESULT SetVertexShaderConstantF(unsigned, const float*, unsigned);
  HRESULT SetPixelShaderConstantF(unsigned, const float*, unsigned);
  HRESULT SetRenderState(D3DRENDERSTATETYPE, DWORD);
  HRESULT SetSamplerState(unsigned, D3DSAMPLERSTATETYPE, DWORD);
  HRESULT SetTextureStageState(unsigned, D3DTEXTURESTAGESTATETYPE, DWORD);
  HRESULT SetRenderTarget(unsigned, BgfxSurface*);
  HRESULT SetDepthStencilSurface(BgfxSurface*);
  HRESULT GetRenderTarget(unsigned, BgfxSurface**);
  HRESULT GetDepthStencilSurface(BgfxSurface**);
  HRESULT Clear(DWORD, const D3DRECT*, DWORD, D3DCOLOR, float, DWORD);
  HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE, int, unsigned, unsigned, unsigned, unsigned);
  HRESULT DrawPrimitive(D3DPRIMITIVETYPE, unsigned start, unsigned count);
  HRESULT UpdateSurface(BgfxSurface*, const RECT*, BgfxSurface*, const POINT*);
  HRESULT BeginScene();
  HRESULT EndScene();
  HRESULT ValidateDevice(DWORD* passes);
  // The fixed-function path is unreachable: bgfx always exposes shaders.
  HRESULT SetFVF(DWORD) { return E_NOTIMPL; }
  HRESULT SetTransform(D3DTRANSFORMSTATETYPE, const D3DMATRIX*) { return E_NOTIMPL; }
  HRESULT SetLight(unsigned, const D3DLIGHT9*) { return E_NOTIMPL; }
  HRESULT LightEnable(unsigned, BOOL) { return E_NOTIMPL; }
  HRESULT SetMaterial(const D3DMATERIAL9*) { return E_NOTIMPL; }
  HRESULT SetSoftwareVertexProcessing(BOOL) { return E_NOTIMPL; }
};
}
