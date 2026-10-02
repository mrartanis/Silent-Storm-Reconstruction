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
};
bool PollInput(InputEvent* event);
bool Active();
bool Exiting();
void Exit();
void Size(int* width, int* height);
const S2Display::Metrics& Display();
void UpdateDisplay(float uiPercent = -1);
bool SetMode(int width, int height, bool fullscreen);
void CursorPosition(float* x, float* y);
void CaptureMouse(bool capture);
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
