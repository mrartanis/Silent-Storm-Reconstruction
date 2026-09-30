#include "StdAfx.h"
#undef for
#include "../Game/Platform.h"
#include <SDL3/SDL_video.h>
#include "../Misc/2DArray.h"
#include "../MiscDll/Commands.h"
#include "../FileIO/BasicChunk1.h"
#include "Gfx.h"
#include "GfxInternal.h"
#include "GfxBuffersInternal.h"

namespace NGfx {
NWin32Helper::com_ptr<BgfxDevice> pDevice;
SRenderStats renderStats;
SRenderTargetsInfo rtInfo;
D3DCAPS9 devCaps{};
bool bHardwareVP = false, bHardwarePixelShaders = false, bHardwarePixelShaders14 = false;
bool bTnLDevice = false, bNVHackNP2 = false, bBanNP2 = false;
bool bBan32BitIndices = true, bStaticNooverwrite = true, bNoCubeMapMipLevels = false;
int nUseAnisotropy = 1, nVCacheSize = 10;
static HWND windowHandle;
static SVideoMode videoMode;
static bool initialized = false, gammaEnabled = false;
bool Is16BitTextures() { return false; }
bool Is16BitMode() { return false; }
int GetMaxAnisotropicLevel() { return 16; }
HWND GetHWND() { return windowHandle; }
CVec2 GetScreenRect() { return CVec2(videoMode.nXSize, videoMode.nYSize); }
void CheckDeviceCaps() {
  bHardwareVP = bHardwarePixelShaders = bHardwarePixelShaders14 = true;
  Zero(devCaps);
  devCaps.MaxTextureWidth = devCaps.MaxTextureHeight = 16384;
  devCaps.MaxVertexIndex = 0x7fffffff;
  devCaps.MaxAnisotropy = 16;
  devCaps.VertexShaderVersion = D3DVS_VERSION(1,1);
  devCaps.PixelShaderVersion = D3DPS_VERSION(1,4);
  devCaps.TextureCaps = D3DPTEXTURECAPS_MIPMAP | D3DPTEXTURECAPS_MIPCUBEMAP | D3DPTEXTURECAPS_CUBEMAP;
}
bool Init3D(HWND handle) {
  windowHandle = handle;
  CheckDeviceCaps();
  return handle != nullptr;
}
bool SetMode(const SVideoMode& mode, const SRenderTargetsInfo& targets) {
  if (mode.nXSize <= 0 || mode.nYSize <= 0 ||
      !S2Platform::SetMode(mode.nXSize, mode.nYSize, mode.fullScreen == FULL_SCREEN)) return false;
  if (initialized) {
    DoneRender(); DestroyLostableBuffers(); DoneZBuffer();
  }
  videoMode = mode;
  rtInfo = targets;
  if (!pDevice) {
    pDevice.Create(new BgfxDevice);
    if (!pDevice->Init(windowHandle, mode.nXSize, mode.nYSize)) { pDevice = 0; return false; }
  } else if (!pDevice->Resize(mode.nXSize, mode.nYSize)) return false;
  if (!InitZBuffer(D3DFMT_D24S8)) return false;
  InitBuffers();
  initialized = SUCCEEDED(InitRender());
  return initialized && pDevice->Healthy();
}
void CheckBackBufferSize() {
  if (!initialized || videoMode.fullScreen != WINDOWED) return;
  int width, height;
  S2Platform::Size(&width, &height);
  if (width > 0 && height > 0 && (width != videoMode.nXSize || height != videoMode.nYSize)) {
    SVideoMode resized = videoMode;
    resized.nXSize = width; resized.nYSize = height;
    SetMode(resized, rtInfo);
  }
}
void GetModesList(list<SVideoMode>* output, int bpp) {
  output->clear();
  if (bpp != 32 || !S2Platform::Window()) return;
  int count = 0;
  SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(SDL_GetDisplayForWindow(S2Platform::Window()), &count);
  for (int i = 0; i < count; ++i) {
    const SDL_DisplayMode& mode = *modes[i];
    bool exists = false;
    for (const auto& old : *output) if (old.nXSize == mode.w && old.nYSize == mode.h) exists = true;
    if (!exists) output->push_back(SVideoMode(mode.w, mode.h, 32, FULL_SCREEN, int(mode.refresh_rate)));
  }
  SDL_free(modes);
}
bool Is3DActive() { return initialized && pDevice && pDevice->Healthy(); }
void SetGamma(bool enabled) { gammaEnabled = enabled; }
void Flip() {
  if (!initialized) return;
  pDevice->Present(gammaEnabled ? Max(0.1f, NGlobal::GetVar("gfx_gamma", 1).GetFloat()) : 1.0f);
  NextFrameBuffes();
  renderStats.Clear();
}
void MakeScreenShot(CArray2D<SPixel8888>* output, bool correctGamma) {
  std::vector<unsigned char> pixels;
  unsigned width = 0, height = 0;
  if (pDevice) pDevice->Screenshot(&pixels, &width, &height);
  output->SetSizes(width ? width : 1, height ? height : 1);
  for (unsigned y = 0; y < height; ++y) memcpy(&(*output)[y][0], pixels.data() + y * width * 4, width * 4);
  if (correctGamma && gammaEnabled) {
    float gamma = Max(0.1f, NGlobal::GetVar("gfx_gamma", 1).GetFloat());
    for (unsigned y=0; y<height; ++y) for (unsigned x=0; x<width; ++x) {
      SPixel8888& pixel = (*output)[y][x];
      pixel.r = static_cast<unsigned char>(pow(pixel.r/255.0f,1.0f/gamma)*255.0f+0.5f);
      pixel.g = static_cast<unsigned char>(pow(pixel.g/255.0f,1.0f/gamma)*255.0f+0.5f);
      pixel.b = static_cast<unsigned char>(pow(pixel.b/255.0f,1.0f/gamma)*255.0f+0.5f);
    }
  }
}
void Done3D() {
  if (initialized) { DoneRender(); DestroyLostableBuffers(); DoneZBuffer(); DestroyManagedBuffers(); }
  initialized = false;
  pDevice = 0;
}
START_REGISTER(Gfx)
  REGISTER_VAR("gfx_tnl_mode", 0, 0, true)
  REGISTER_VAR("gfx_gamma", 0, 1, true)
  REGISTER_VAR_EX("gfx_anisotropic_filter", NGlobal::VarIntHandler, &nUseAnisotropy, 1, true)
  REGISTER_VAR_EX("gfx_validate", NGlobal::VarBoolHandler, &bDoValidateDevice, 0, true)
FINISH_REGISTER
}
