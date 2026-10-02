#pragma once
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <string>
#include <vector>

namespace S2Display {
struct Metrics {
  int windowWidth = 1024, windowHeight = 768;
  int pixelWidth = 1024, pixelHeight = 768;
  float displayScale = 1, uiScale = 1;
  bool drawable = true;
  unsigned revision = 0;
  unsigned displayID = 0;
  float CanvasWidth() const { return pixelWidth / uiScale; }
  float CanvasHeight() const { return pixelHeight / uiScale; }
  float WindowToPixelX(float x) const { return x * pixelWidth / (std::max)(1, windowWidth); }
  float WindowToPixelY(float y) const { return y * pixelHeight / (std::max)(1, windowHeight); }
  float PixelToUI(float x) const { return x / uiScale; }
  float UIToPixel(float x) const { return x * uiScale; }
};
inline float UIScale(int width, int height, float display, float percent) {
  if (width <= 0 || height <= 0) return 1;
  if (!std::isfinite(display) || display <= 0) display = 1;
  if (!std::isfinite(percent) || percent < 0) percent = 0;
  const float requested = percent > 0 ? display * percent / 100 : (std::max)(display, height / 1080.0f);
  return (std::min)(requested, (std::min)(width / 1024.0f, height / 768.0f));
}
inline Metrics Update(const Metrics& previous, int width, int height, int pixelsWide, int pixelsHigh,
    float displayScale, unsigned displayID, bool minimized, float percent) {
  Metrics next=previous;
  next.drawable=pixelsWide>0 && pixelsHigh>0 && !minimized;
  if(width>0 && height>0) { next.windowWidth=width; next.windowHeight=height; }
  if(pixelsWide>0 && pixelsHigh>0) { next.pixelWidth=pixelsWide; next.pixelHeight=pixelsHigh; }
  next.displayScale=std::isfinite(displayScale) && displayScale>0 ? displayScale : 1;
  next.displayID=displayID;
  next.uiScale=UIScale(next.pixelWidth,next.pixelHeight,next.displayScale,percent);
  if(next.windowWidth!=previous.windowWidth || next.windowHeight!=previous.windowHeight ||
      next.pixelWidth!=previous.pixelWidth || next.pixelHeight!=previous.pixelHeight ||
      next.displayScale!=previous.displayScale || next.uiScale!=previous.uiScale ||
      next.drawable!=previous.drawable || next.displayID!=previous.displayID) ++next.revision;
  return next;
}
// Logical dialogue progress survives a different number of wrapped pages.
struct PageAnchor { int phrase, offset; };
inline int PageForSource(const std::vector<PageAnchor>& pages, int phrase, int offset) {
  int result=-1;
  for (std::size_t i=0;i<pages.size();++i) if(pages[i].phrase==phrase) {
    if(result<0 || pages[i].offset<=offset) result=static_cast<int>(i);
    if(pages[i].offset>offset) break;
  }
  return result;
}
struct Rect { float x, y, width, height; };
inline Rect Fit(float width, float height, float sourceWidth, float sourceHeight) {
  if (width <= 0 || height <= 0 || sourceWidth <= 0 || sourceHeight <= 0) return {0,0,0,0};
  const float scale = (std::min)(width / sourceWidth, height / sourceHeight);
  const float w = sourceWidth * scale, h = sourceHeight * scale;
  return {(width-w)/2, (height-h)/2, w, h};
}
// An authored horizontal FOV is measured at the original 4:3 viewport.
inline float HorizontalFOV(float originalDegrees, float width, float height) {
  if (width <= 0 || height <= 0 || width * 3 == height * 4) return originalDegrees;
  constexpr float pi = 3.14159265358979323846f;
  return 360 / pi * std::atan(std::tan(originalDegrees*pi/360) * (width/height) / (4.0f/3));
}
// Preserve authored preview zoom while a widget or the surrounding canvas changes.
inline float PreviewFOV(float originalDegrees, float canvasHeight, float authoredHeight, float viewportHeight) {
  if (canvasHeight <= 0 || authoredHeight <= 0 || viewportHeight <= 0) return originalDegrees;
  const float ratio = canvasHeight / 768 * authoredHeight / viewportHeight;
  if (ratio == 1) return originalDegrees;
  constexpr float pi = 3.14159265358979323846f;
  return 360 / pi * std::atan(std::tan(originalDegrees*pi/360) * ratio);
}

// Matches floor(pixel / scale) used by the integer event API.
inline int Boundary(float logical, float scale) { return static_cast<int>(std::ceil(logical * scale)); }

// Expand a baked HUD backdrop by inserting a neutral column between its fixed ends.
// Source artwork and control frames keep their original widths at any canvas size.
struct HorizontalSlice { int position, width, source, sourceWidth; };
inline std::vector<HorizontalSlice> ExpandHorizontal(int width, int authoredWidth, int split) {
  if (width <= 0 || authoredWidth <= 0) return {};
  if (width <= authoredWidth || split <= 0 || split >= authoredWidth)
    return {{0, width, 0, width}};
  return {{0, split, 0, split}, {split, width-authoredWidth, split-1, 1},
          {split+width-authoredWidth, authoredWidth-split, split, authoredWidth-split}};
}

// Preserve end caps while expanding a single continuous center strip.
inline std::vector<HorizontalSlice> StretchHorizontal(int width,int authoredWidth,int cap) {
  if(width<=0 || authoredWidth<=0) return {};
  if(width<=authoredWidth || cap<=0 || 2*cap>=authoredWidth) return {{0,width,0,width}};
  return {{0,cap,0,cap},{cap,width-2*cap,cap,authoredWidth-2*cap},
          {width-cap,cap,authoredWidth-cap,cap}};
}

// Material-only strip from the empty HUD plate, before the right command frames.
inline HorizontalSlice NeutralHorizontalTile(int textureWidth, int leftExtent) {
  const int limit=(std::min)(textureWidth,leftExtent);
  if(limit<=0) return {0,0,0,0};
  const int width=(std::max)(1,(std::min)(256,limit/2));
  const int source=(std::min)(128,limit-width);
  return {0,width,source,width};
}

enum Anchor { Near, Center, Far, Stretch };
inline Anchor InferAnchor(int position, int size, int extent) {
  if (extent <= 0) return Near;
  if (size >= extent - 16 && position <= 16) return Stretch;
  const float center = position + size * 0.5f;
  if (center < extent / 3.0f) return Near;
  if (center > extent * 2.0f / 3) return Far;
  return Center;
}
inline int Position(int original, int originalExtent, int extent, Anchor anchor) {
  if (anchor == Far) return original + extent - originalExtent;
  if (anchor == Center) return original + (extent - originalExtent) / 2;
  return original;
}
inline bool ParseResolution(const std::wstring& value, int* width, int* height) {
  if (value.size() >= 2 && ((value.front() == L'"' && value.back() == L'"') ||
      (value.front() == L'\'' && value.back() == L'\'')))
    return ParseResolution(value.substr(1,value.size()-2), width, height);
  int w = 0, h = 0;
  wchar_t* dimensionEnd = nullptr;
  const long wide = std::wcstol(value.c_str(), &dimensionEnd, 10);
  if (dimensionEnd != value.c_str() && *dimensionEnd == L'x') {
    const wchar_t* heightStart = dimensionEnd + 1;
    wchar_t* heightEnd = nullptr;
    const long high = std::wcstol(heightStart, &heightEnd, 10);
    while (*heightEnd == L' ' || *heightEnd == L'\t') ++heightEnd;
    if (heightEnd == heightStart || *heightEnd || wide < 100 || high < 100 || wide > 16384 || high > 16384) return false;
    w = static_cast<int>(wide); h = static_cast<int>(high);
  } else {
    wchar_t* end = nullptr;
    const double legacy = std::wcstod(value.c_str(), &end);
    if (end == value.c_str()) return false;
    while (*end == L' ' || *end == L'\t') ++end;
    if (*end || !std::isfinite(legacy) || legacy < 320 || legacy > 1600 || legacy != std::floor(legacy)) return false;
    switch (static_cast<int>(legacy)) {
      case 320: w=320; h=200; break; case 400: w=400; h=300; break;
      case 640: w=640; h=480; break; case 800: w=800; h=600; break;
      case 1024: w=1024; h=768; break; case 1152: w=1152; h=864; break;
      case 1280: w=1280; h=960; break; case 1600: w=1600; h=1200; break;
      default: return false;
    }
  }
  if (w < 100 || h < 100 || w > 16384 || h > 16384) return false;
  *width = w; *height = h; return true;
}
}
