#include "Platform.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <clocale>
#include <deque>
#include <map>
#include <tuple>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {
SDL_Window* window = nullptr;
bool exiting = false;
bool active = false;
bool errorDialogs = true;
S2Display::Metrics display;
float uiPercent = 0;
std::deque<S2Platform::InputEvent> input;
bool nativeCursor = false, mouseCaptured = false, cursorVisible = false;
bool cameraDragAllowed = false;
Uint32 cameraButtons = 0;
float dragX = 0, dragY = 0;
using CursorKey = std::tuple<int,int,int,int,int>;
std::map<CursorKey,SDL_Cursor*> cursors;
S2Platform::CursorStats cursorStats{};
void ApplyMouseMode()
{
  if (!window) return;
  const bool relative = mouseCaptured && (!nativeCursor || cameraButtons != 0);
  if (SDL_GetWindowRelativeMouseMode(window) != relative)
    SDL_SetWindowRelativeMouseMode(window, relative);
  SDL_SetWindowMouseGrab(window, mouseCaptured);
  SDL_CaptureMouse(mouseCaptured);
  const bool visible = !mouseCaptured || (nativeCursor && cursorVisible && !cameraButtons);
  if (SDL_CursorVisible() != visible) { if (visible) SDL_ShowCursor(); else SDL_HideCursor(); }
}
void StopCameraDrag(bool restore)
{
  if (!cameraButtons) return;
  cameraButtons = 0;
  ApplyMouseMode();
  if (restore && window && active) SDL_WarpMouseInWindow(window, dragX, dragY);
}
void QueueMouse(const SDL_Event& event)
{
  S2Platform::InputEvent item{event,0};
  if (nativeCursor) {
    item.hasPointer = true;
    float x = dragX, y = dragY;
    if (!cameraButtons) {
      if (event.type == SDL_EVENT_MOUSE_MOTION) { x=event.motion.x; y=event.motion.y; }
      else { x=event.button.x; y=event.button.y; }
    }
    item.pointerX = display.WindowToPixelX(x);
    item.pointerY = display.WindowToPixelY(y);
  }
  input.push_back(item);
}
void InitError(const char* operation)
{
  char message[1024];
  SDL_snprintf(message, sizeof(message), "%s: %s", operation, SDL_GetError());
  S2Platform::Error(message);
}
}

bool S2Platform::Init(const char* title, int width, int height, bool hidden)
{
  Done();
  exiting = false;
#if !defined(_WIN32)
  // Wide-character UI classification must recognize Cyrillic even when the
  // launcher has no LANG. Leave the numeric locale unchanged for game data.
  if (!std::setlocale(LC_CTYPE, "C.UTF-8")) {
    S2Platform::Error("Unicode character locale C.UTF-8 is unavailable");
    return false;
  }
#endif
  SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");
  // Cursor images are already sized to drawable/UI pixels; avoid a second DPI scale.
  SDL_SetHint(SDL_HINT_MOUSE_DPI_SCALE_CURSORS, "0");
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    InitError("SDL_Init");
    SDL_Quit();
    return false;
  }
  window = SDL_CreateWindow(title, width, height,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | (hidden ? SDL_WINDOW_HIDDEN : 0));
  if (!window) {
    InitError("SDL_CreateWindow");
    SDL_Quit();
    return false;
  }
  SDL_SetWindowMinimumSize(window, 100, 100);
  active = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
  UpdateDisplay();
  if (!SDL_StartTextInput(window)) {
    InitError("SDL_StartTextInput");
    Done();
    return false;
  }
  return true;
}

void S2Platform::Done()
{
  input.clear();
  if (window) {
    CaptureMouse(false);
    SDL_SetCursor(SDL_GetDefaultCursor());
    for (auto& entry : cursors) if (entry.second) SDL_DestroyCursor(entry.second);
    cursors.clear(); cursorStats = {};
    SDL_StopTextInput(window);
    SDL_DestroyWindow(window);
    window = nullptr;
    SDL_Quit();
  }
  active = false;
  nativeCursor = mouseCaptured = cursorVisible = cameraDragAllowed = false;
  cameraButtons = 0;
}

SDL_Window* S2Platform::Window() { return window; }
void* S2Platform::NativeWindow()
{
#if defined(_WIN32)
  return window ? SDL_GetPointerProperty(SDL_GetWindowProperties(window),
      SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr) : nullptr;
#else
  if (!window) return nullptr;
  const auto properties = SDL_GetWindowProperties(window);
  if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0)
    return SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
  return reinterpret_cast<void*>(static_cast<std::uintptr_t>(SDL_GetNumberProperty(
      properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0)));
#endif
}

void* S2Platform::NativeDisplay()
{
#if defined(_WIN32)
  return nullptr;
#else
  if (!window) return nullptr;
  const auto properties = SDL_GetWindowProperties(window);
  return SDL_GetPointerProperty(properties,
      SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0
          ? SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER : SDL_PROP_WINDOW_X11_DISPLAY_POINTER,
      nullptr);
#endif
}

void S2Platform::PumpEvents()
{
  SDL_Event event;
  const SDL_WindowID id = window ? SDL_GetWindowID(window) : 0;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) { exiting = true; continue; }
    // Window/input events have windowID at the same offset in SDL's union.
    if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST) {
      if (event.window.windowID != id) continue;
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) exiting = true;
      if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST ||
          event.type == SDL_EVENT_WINDOW_MINIMIZED || event.type == SDL_EVENT_WINDOW_HIDDEN) {
        active = false;
        CaptureMouse(false);
        input.push_back({event, 0});
      }
      if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
        active = true;
        input.push_back({event, 0});
      }
      if (event.type == SDL_EVENT_WINDOW_RESTORED)
        active = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
      continue;
    }
    switch (event.type) {
      case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP:
        if (event.key.windowID == id) input.push_back({event, 0});
        break;
      case SDL_EVENT_TEXT_INPUT:
        if (event.text.windowID == id) {
          // SDL owns text.text only until the next poll. Copy it as key events
          // carrying Unicode scalars, preserving its position in the event stream.
          const unsigned char* p = reinterpret_cast<const unsigned char*>(event.text.text);
          while (p && *p) {
            std::uint32_t code = *p++;
            int extra = 0;
            if (code >= 0xc2 && code <= 0xdf) { code &= 0x1f; extra = 1; }
            else if (code >= 0xe0 && code <= 0xef) { code &= 0x0f; extra = 2; }
            else if (code >= 0xf0 && code <= 0xf4) { code &= 7; extra = 3; }
            else if (code >= 0x80) { continue; }
            bool valid = true;
            const int length = extra;
            while (extra--) {
              if ((*p & 0xc0) != 0x80) { valid = false; break; }
              code = (code << 6) | (*p++ & 0x3f);
            }
            if (!valid || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff) ||
                (length == 1 && code < 0x80) || (length == 2 && code < 0x800) ||
                (length == 3 && code < 0x10000)) continue;
            SDL_Event scalar{};
            scalar.type = SDL_EVENT_TEXT_INPUT;
            input.push_back({scalar, code});
          }
        }
        break;
      case SDL_EVENT_MOUSE_MOTION:
        if (event.motion.windowID == id) QueueMouse(event);
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.button.windowID == id) {
          QueueMouse(event);
          const Uint32 bit=SDL_BUTTON_MASK(event.button.button);
          if (nativeCursor && cameraDragAllowed && mouseCaptured &&
              (event.button.button == SDL_BUTTON_RIGHT || event.button.button == SDL_BUTTON_MIDDLE)) {
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
              if (!cameraButtons) { dragX=event.button.x; dragY=event.button.y; }
              cameraButtons |= bit; ApplyMouseMode();
            } else {
              const Uint32 remaining=cameraButtons & ~bit;
              if (remaining) cameraButtons=remaining;
              else StopCameraDrag(true);
            }
          }
        }
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        if (event.wheel.windowID == id) input.push_back({event, 0});
        break;
      default: break;
    }
  }
  UpdateDisplay();
}

bool S2Platform::PollInput(InputEvent* event)
{
  if (input.empty()) return false;
  *event = input.front(); input.pop_front(); return true;
}
bool S2Platform::Active() { return active; }
bool S2Platform::Exiting() { return exiting; }
void S2Platform::Exit() { exiting = true; }
void S2Platform::Size(int* width, int* height)
{
  *width = display.drawable ? display.pixelWidth : 0;
  *height = display.drawable ? display.pixelHeight : 0;
}
const S2Display::Metrics& S2Platform::Display() { return display; }
void S2Platform::UpdateDisplay(float percent)
{
  if (percent >= 0 && std::isfinite(percent)) uiPercent = percent;
  if (!window) return;
  int w = 0, h = 0, pw = 0, ph = 0;
  SDL_GetWindowSize(window, &w, &h);
  SDL_GetWindowSizeInPixels(window, &pw, &ph);
  display = S2Display::Update(display,w,h,pw,ph,SDL_GetWindowDisplayScale(window),
      SDL_GetDisplayForWindow(window),bool(SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED),uiPercent);
}
bool S2Platform::SetMode(int width, int height, bool fullscreen)
{
  SDL_DisplayMode mode{};
  if (fullscreen && (!window || !SDL_GetClosestFullscreenDisplayMode(
      SDL_GetDisplayForWindow(window), width, height, 0, true, &mode) ||
      mode.w != width || mode.h != height)) {
    std::fprintf(stderr,"DISPLAY: fullscreen resolution %dx%d is unavailable\n",width,height);
    return false;
  }
  if (!window || !SDL_SetWindowFullscreenMode(window, fullscreen ? &mode : nullptr) ||
      !SDL_SetWindowFullscreen(window, fullscreen) ||
      (!fullscreen && !SDL_SetWindowSize(window, width, height)) || !SDL_SyncWindow(window)) {
    std::fprintf(stderr,"DISPLAY: SDL mode change failed: %s\n",SDL_GetError());
    return false;
  }
  UpdateDisplay();
  return true;
}
void S2Platform::CursorPosition(float* x, float* y) {
  if (cameraButtons) { *x=dragX; *y=dragY; }
  else SDL_GetMouseState(x, y);
  *x = display.WindowToPixelX(*x); *y = display.WindowToPixelY(*y);
}
void S2Platform::CaptureMouse(bool capture)
{
  if (!window) return;
  if (!capture) StopCameraDrag(false);
  mouseCaptured = capture;
  ApplyMouseMode();
}
void S2Platform::UseNativeCursor(bool enabled)
{
  if (nativeCursor == enabled) return;
  StopCameraDrag(true);
  nativeCursor=enabled; ApplyMouseMode();
}
bool S2Platform::NativeCursorEnabled() { return nativeCursor; }
void S2Platform::AllowCameraMouseDrag(bool allowed)
{
  if (!allowed) StopCameraDrag(true);
  cameraDragAllowed=allowed;
}
bool S2Platform::CameraMouseDragging() { return cameraButtons != 0; }
bool S2Platform::SelectNativeCursor(int id,int width,int height,int hotX,int hotY,
    const void* pixels,int sourceWidth,int sourceHeight)
{
  if (!window || width<=0 || height<=0 || width>512 || height>512 ||
      hotX<0 || hotY<0 || hotX>=width || hotY>=height) return false;
  const CursorKey key{id,width,height,hotX,hotY};
  auto it=cursors.find(key);
  if (it==cursors.end()) {
    if (!pixels || sourceWidth<=0 || sourceHeight<=0 || sourceWidth>4096 || sourceHeight>4096) return false;
    SDL_Surface* source=SDL_CreateSurfaceFrom(sourceWidth,sourceHeight,SDL_PIXELFORMAT_ARGB8888,
        const_cast<void*>(pixels),sourceWidth*4);
    SDL_Surface* scaled=SDL_CreateSurface(width,height,SDL_PIXELFORMAT_ARGB8888);
    SDL_Cursor* cursor=nullptr;
    if (source && scaled) {
      SDL_SetSurfaceBlendMode(source,SDL_BLENDMODE_NONE);
      if (SDL_BlitSurfaceScaled(source,nullptr,scaled,nullptr,SDL_SCALEMODE_LINEAR)) {
        cursor=SDL_CreateColorCursor(scaled,hotX,hotY);
        if (const char* dir=SDL_getenv("S2_DIAG_CURSOR_DIR")) {
          const std::string path=std::string(dir)+"/cursor-"+std::to_string(id)+"-"+std::to_string(width)+"x"+std::to_string(height)+".bmp";
          SDL_SaveBMP(scaled,path.c_str());
        }
      }
    }
    SDL_DestroySurface(source); SDL_DestroySurface(scaled);
    if (!cursor) std::fprintf(stderr,"CURSOR: creation failed id=%d: %s\n",id,SDL_GetError());
    it=cursors.emplace(key,cursor).first;
    if (cursor) ++cursorStats.creations;
  }
  if (!it->second) return false;
  if (SDL_GetCursor()!=it->second && !SDL_SetCursor(it->second)) return false;
  cursorStats.id=id; cursorStats.width=width; cursorStats.height=height;
  cursorStats.hotX=hotX; cursorStats.hotY=hotY;
  return true;
}
void S2Platform::NativeCursorVisible(bool visible)
{
  if (cursorVisible==visible) return;
  cursorVisible=visible; ApplyMouseMode();
}
S2Platform::CursorStats S2Platform::GetCursorStats()
{
  auto result=cursorStats; result.cached=int(cursors.size()); result.native=nativeCursor;
  result.dragging=cameraButtons!=0; result.visible=SDL_CursorVisible(); return result;
}
void S2Platform::Error(const char* message)
{
  std::fprintf(stderr, "Silent Storm: %s\n", message);
  if (errorDialogs) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Silent Storm", message, window);
}
void S2Platform::SetErrorDialogs(bool enabled) { errorDialogs = enabled; }
bool S2Platform::ControlPressed() { return (SDL_GetModState() & SDL_KMOD_CTRL) != 0; }
std::wstring S2Platform::ClipboardText()
{
  char* utf8 = SDL_GetClipboardText();
  if (!utf8) return {};
  char* wide = SDL_iconv_string(sizeof(wchar_t) == 2 ? "UTF-16LE" : "UTF-32LE",
      "UTF-8", utf8, SDL_strlen(utf8) + 1);
  std::wstring result;
  if (wide) { result = reinterpret_cast<const wchar_t*>(wide); SDL_free(wide); }
  SDL_free(utf8);
  return result;
}
std::uint32_t S2Platform::DoubleClickMilliseconds()
{
#if defined(_WIN32)
  return GetDoubleClickTime();
#else
  const char* value = SDL_GetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_TIME);
  return value ? static_cast<std::uint32_t>(SDL_atoi(value)) : 500;
#endif
}
void S2Platform::MouseAcceleration(int* threshold1, int* threshold2, int* acceleration)
{
  int parameters[3] = {6, 10, 1};
#if defined(_WIN32)
  SystemParametersInfo(SPI_GETMOUSE, 0, parameters, 0);
#endif
  *threshold1 = parameters[0]; *threshold2 = parameters[1]; *acceleration = parameters[2];
}
std::uint32_t S2Platform::Milliseconds() { return static_cast<std::uint32_t>(SDL_GetTicks()); }
void S2Platform::Delay(std::uint32_t milliseconds) { SDL_Delay(milliseconds); }

std::uint64_t S2Platform::PhysicalMemoryBytes() {
  const int megabytes = SDL_GetSystemRAM();
  return megabytes > 0 ? std::uint64_t(megabytes) * 1024 * 1024 : 0;
}
