#pragma once
#include "../Game/Platform.h"
namespace S2UI {
inline float Scale() { return S2Platform::Display().uiScale; }
// The integer window API must also cover the final fractional logical pixel.
inline int Width() { return static_cast<int>(std::ceil(S2Platform::Display().CanvasWidth())); }
inline int Height() { return static_cast<int>(std::ceil(S2Platform::Display().CanvasHeight())); }
// Campaign map data is authored in screen coordinates on a centered 1024x768 form.
inline int MapOffsetX() { return (Width()-1024)/2; }
inline int MapOffsetY() { return (Height()-768)/2; }
inline float ToPixel(float x) { return x * Scale(); }
inline float FromPixel(float x) { return x / Scale(); }
}
