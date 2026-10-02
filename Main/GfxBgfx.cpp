#include "StdAfx.h"
#undef for
#include "../Game/Platform.h"
#include <SDL3/SDL_video.h>
#include "../Misc/2DArray.h"
#include "../MiscDll/Commands.h"
#include "../FileIO/BasicChunk1.h"
#include "Gfx.h"
#include "VectorFonts.h"
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
// bgfx exposes anisotropy as off / the device maximum, not individual levels.
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
  const auto& display = S2Platform::Display();
  videoMode.nXSize = display.pixelWidth; videoMode.nYSize = display.pixelHeight;
  rtInfo = targets;
  if (!pDevice) {
    pDevice.Create(new BgfxDevice);
    if (!pDevice->Init(windowHandle, videoMode.nXSize, videoMode.nYSize)) { pDevice = 0; return false; }
  } else if (!pDevice->Resize(videoMode.nXSize, videoMode.nYSize)) return false;
  if (!InitZBuffer(D3DFMT_D24S8)) return false;
  InitBuffers();
  initialized = SUCCEEDED(InitRender());
  return initialized && pDevice->Healthy();
}
void CheckBackBufferSize() {
  if (!initialized) return;
  int width, height;
  S2Platform::Size(&width, &height);
  if (width > 0 && height > 0 && (width != videoMode.nXSize || height != videoMode.nYSize)) {
    DoneRender(); DestroyLostableBuffers(); DoneZBuffer();
    videoMode.nXSize = width; videoMode.nYSize = height;
    initialized = pDevice->Resize(width, height) && InitZBuffer(D3DFMT_D24S8);
    if (initialized) { InitBuffers(); initialized = SUCCEEDED(InitRender()); }
  }
}
void GetModesList(list<SVideoMode>* output, int bpp) {
  output->clear();
  if (bpp != 32 || !S2Platform::Window()) return;
  int count = 0;
  SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(SDL_GetDisplayForWindow(S2Platform::Window()), &count);
  for (int i = 0; modes && i < count; ++i) {
    const SDL_DisplayMode& mode = *modes[i];
    bool exists = false;
    for (const auto& old : *output) if (old.nXSize == mode.w && old.nYSize == mode.h) exists = true;
    if (!exists) output->push_back(SVideoMode(mode.w, mode.h, 32, FULL_SCREEN, int(mode.refresh_rate)));
  }
  SDL_free(modes);
  const auto& display = S2Platform::Display();
  bool exists = false;
  for (const auto& old : *output) if (old.nXSize == display.windowWidth && old.nYSize == display.windowHeight) exists = true;
  if (!exists) output->push_back(SVideoMode(display.windowWidth, display.windowHeight, 32, WINDOWED));
  output->sort([](const SVideoMode& a, const SVideoMode& b) {
    return a.nXSize < b.nXSize || (a.nXSize == b.nXSize && a.nYSize < b.nYSize);
  });
}
bool Is3DActive() { return initialized && pDevice && pDevice->Healthy(); }
bool GetDesktopVideoMode(SVideoMode* output) {
  if(!output || !S2Platform::Window())return false;
  const SDL_DisplayMode* mode=SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(S2Platform::Window()));
  if(!mode)return false;
  *output=SVideoMode(mode->w,mode->h,32,FULL_SCREEN,int(mode->refresh_rate));
  return true;
}
void SetGamma(bool enabled) { gammaEnabled = enabled; }
void ApplySceneAntialiasing() {
  if(initialized && pDevice) pDevice->ApplySceneAntialiasing();
}
void Flip() {
  if (!initialized || !S2Platform::Display().drawable) return;
  pDevice->Present(gammaEnabled ? Max(0.1f, NGlobal::GetVar("gfx_gamma", 1).GetFloat()) : 1.0f);
  NextFrameBuffes();
  renderStats.Clear();
}
void MakeScreenShot(CArray2D<SPixel8888>* output, bool correctGamma) {
  std::vector<unsigned char> pixels;
  unsigned width = 0, height = 0;
  if (pDevice) pDevice->Screenshot(&pixels, &width, &height);
  // A save thumbnail bilinearly samples neighbouring pixels. A renderer
  // failure must produce an initialized image with valid neighbours, not 1x1.
  if(!width || !height || pixels.size()<size_t(width)*height*4) {
    output->SetSizes(Max(2,videoMode.nXSize),Max(2,videoMode.nYSize));
    for(int y=0;y<output->GetYSize();++y)for(int x=0;x<output->GetXSize();++x)
      (*output)[y][x]=SPixel8888(0,0,0,255);
    return;
  }
  output->SetSizes(width,height);
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
  NGScene::ResetVectorFontDevice();
  if (initialized) { DoneRender(); DestroyLostableBuffers(); DoneZBuffer(); DestroyManagedBuffers(); }
  initialized = false;
  pDevice = 0;
}
START_REGISTER(Gfx)
  REGISTER_VAR("gfx_tnl_mode", 0, 0, true)
  REGISTER_VAR("gfx_gamma", 0, 1, true)
  REGISTER_VAR_EX("gfx_anisotropic_filter", NGlobal::VarIntHandler, &nUseAnisotropy, 1, true)
  REGISTER_VAR("gfx_vsync", 0, 1, true)
  REGISTER_VAR("gfx_antialiasing", 0, 0, true) // 0 = off, 1 = FXAA on the 3D scene before UI.
  REGISTER_VAR_EX("gfx_validate", NGlobal::VarBoolHandler, &bDoValidateDevice, 0, true)
FINISH_REGISTER
}
