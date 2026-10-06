#pragma once
#include <cstdint>
#include <string>
#include <SDL3/SDL_events.h>
#include "DisplayGeometry.h"

struct SDL_Window;

// SDL is owned by the application thread; input is drained once per frame.
namespace S2Platform {
bool Init(const char* title, int width = 1024, int height = 768, bool hidden = false);
void Done();
SDL_Window* Window();
void* NativeWindow(); // Native window/surface supplied to the graphics backend.
void* NativeDisplay();
void PumpEvents();
struct InputEvent {
  SDL_Event event{};
  std::uint32_t character = 0;
  bool hasPointer = false;
  float pointerX = 0, pointerY = 0; // drawable pixels, captured at the event
};
bool PollInput(InputEvent* event);
bool Active();
bool Exiting();
void Exit();
void Size(int* width, int* height);
const S2Display::Metrics& Display();
void UpdateDisplay(float uiPercent = -1);
enum class WindowMode { Windowed = 0, Fullscreen = 1, Borderless = 2 };
bool SetMode(int width, int height, WindowMode mode);
inline bool SetMode(int width, int height, bool fullscreen) {
  return SetMode(width,height,fullscreen?WindowMode::Fullscreen:WindowMode::Windowed);
}
void CursorPosition(float* x, float* y);
void CaptureMouse(bool capture);
void UseNativeCursor(bool enabled);
bool NativeCursorEnabled();
void AllowCameraMouseDrag(bool allowed);
bool CameraMouseDragging();
// A null image selects an already cached cursor; pixels are top-down ARGB8888.
bool SelectNativeCursor(int id, int width, int height, int hotX, int hotY,
    const void* pixels = nullptr, int sourceWidth = 0, int sourceHeight = 0);
void NativeCursorVisible(bool visible);
struct CursorStats { int id, width, height, hotX, hotY, cached, creations; bool native, dragging, visible; };
CursorStats GetCursorStats();
bool ControlPressed();
std::wstring ClipboardText();
std::uint32_t DoubleClickMilliseconds();
void MouseAcceleration(int* threshold1, int* threshold2, int* acceleration);
void Error(const char* message);
void SetErrorDialogs(bool enabled); // Unattended diagnostics keep errors in stderr.
std::uint64_t PhysicalMemoryBytes();
std::uint32_t Milliseconds();
void Delay(std::uint32_t milliseconds);
}
