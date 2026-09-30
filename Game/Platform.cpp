#include "Platform.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <deque>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {
SDL_Window* window = nullptr;
bool exiting = false;
bool active = false;
bool errorDialogs = true;
std::deque<S2Platform::InputEvent> input;
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
  SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    InitError("SDL_Init");
    SDL_Quit();
    return false;
  }
  window = SDL_CreateWindow(title, width, height,
      SDL_WINDOW_RESIZABLE | (hidden ? SDL_WINDOW_HIDDEN : 0));
  if (!window) {
    InitError("SDL_CreateWindow");
    SDL_Quit();
    return false;
  }
  SDL_SetWindowMinimumSize(window, 100, 100);
  active = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
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
    SDL_StopTextInput(window);
    SDL_DestroyWindow(window);
    window = nullptr;
    SDL_Quit();
  }
  active = false;
}

SDL_Window* S2Platform::Window() { return window; }
void* S2Platform::NativeWindow()
{
#if defined(_WIN32)
  return window ? SDL_GetPointerProperty(SDL_GetWindowProperties(window),
      SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr) : nullptr;
#else
  return nullptr;
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
        if (event.motion.windowID == id) input.push_back({event, 0});
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event.button.windowID == id) input.push_back({event, 0});
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        if (event.wheel.windowID == id) input.push_back({event, 0});
        break;
      default: break;
    }
  }
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
  if (!window || !SDL_GetWindowSizeInPixels(window, width, height)) *width = *height = 0;
}
bool S2Platform::SetMode(int width, int height, bool fullscreen)
{
  return window && SDL_SetWindowFullscreen(window, fullscreen) &&
      (fullscreen || SDL_SetWindowSize(window, width, height));
}
void S2Platform::CursorPosition(float* x, float* y) { SDL_GetMouseState(x, y); }
void S2Platform::CaptureMouse(bool capture)
{
  if (!window) return;
  SDL_SetWindowMouseGrab(window, capture);
  SDL_SetWindowRelativeMouseMode(window, capture);
  SDL_CaptureMouse(capture);
  if (capture) SDL_HideCursor(); else SDL_ShowCursor();
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
