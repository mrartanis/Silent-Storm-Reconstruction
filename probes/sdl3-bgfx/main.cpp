#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace {
constexpr int kWidth = 640;
constexpr int kHeight = 360;

bool attachWindow(SDL_Window* window, bgfx::Init& init)
{
  const SDL_PropertiesID props = SDL_GetWindowProperties(window);
  const char* driver = SDL_GetCurrentVideoDriver();
  if (std::strcmp(driver, "windows") == 0) {
    init.swapChain.nwh = SDL_GetPointerProperty(
        props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
  } else if (std::strcmp(driver, "x11") == 0) {
    init.swapChain.ndt = SDL_GetPointerProperty(
        props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    init.swapChain.nwh = reinterpret_cast<void*>(static_cast<uintptr_t>(
        SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0)));
  } else if (std::strcmp(driver, "wayland") == 0) {
    init.swapChain.ndt = SDL_GetPointerProperty(
        props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
    init.swapChain.nwh = SDL_GetPointerProperty(
        props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
  } else if (std::strcmp(driver, "cocoa") == 0) {
    init.swapChain.nwh = SDL_GetPointerProperty(
        props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
  } else {
    return false;
  }
  return init.swapChain.nwh != nullptr;
}
} // namespace

int main(int argc, char** argv)
{
  bool headless = false;
  int frames = 120;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--headless") == 0) {
      headless = true;
    } else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
      frames = std::atoi(argv[++i]);
      if (frames < 1 || frames > 3600) return 2;
    } else {
      std::fprintf(stderr, "usage: S2SDL3BgfxProbe [--headless] [--frames 1..3600]\n");
      return 2;
    }
  }
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
    return 1;
  }
  SDL_Window* window = SDL_CreateWindow("Silent Storm SDL3 + bgfx probe",
      kWidth, kHeight, headless ? SDL_WINDOW_HIDDEN : 0);
  if (!window) {
    std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  bgfx::Init init;
  init.type = headless ? bgfx::RendererType::Noop : bgfx::RendererType::Count;
  init.swapChain.width = kWidth;
  init.swapChain.height = kHeight;
  init.reset = BGFX_RESET_VSYNC;
  if (!headless && !attachWindow(window, init)) {
    std::fprintf(stderr, "No native window handle for SDL driver %s\n",
                 SDL_GetCurrentVideoDriver());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  if (!bgfx::init(init)) {
    std::fprintf(stderr, "bgfx::init failed\n");
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  std::printf("SDL driver=%s, bgfx renderer=%s\n",
              SDL_GetCurrentVideoDriver(), bgfx::getRendererName(bgfx::getRendererType()));
  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x25314bff, 1.0f, 0);
  bgfx::setViewRect(0, 0, 0, kWidth, kHeight);
  int submitted = 0;
  for (int i = 0; i < (headless ? 3 : frames); ++i) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) i = frames;
    }
    if (i >= frames) break;
    bgfx::touch(0);
    bgfx::frame();
    ++submitted;
    if (!headless) SDL_Delay(16);
  }
  std::printf("submitted %d frames\n", submitted);
  bgfx::shutdown();
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
