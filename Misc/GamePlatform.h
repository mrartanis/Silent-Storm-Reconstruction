#pragma once
#include "../FileIO/HeadlessPlatform.h"
#include "Tools.h"
#include "Basic2.h"
#include "Geom.h"
#include "../FileIO/BasicChunk1.h"
#include <cstdint>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstdlib>
#include <cwchar>

// Fixed-width values retained by the game-facing graphics and media adapters.
// They describe data; no Windows service or Direct3D runtime is involved.
using UINT = unsigned;
using WCHAR = wchar_t;
using INT = int;
using LONG = std::int32_t;
using HRESULT = std::int32_t;
using HWND = void*;
using HANDLE = void*;
struct RECT { LONG left, top, right, bottom; };
struct POINT { LONG x, y; };
constexpr HRESULT S_OK = 0, E_FAIL = static_cast<HRESULT>(0x80004005u);
constexpr HRESULT E_INVALIDARG = static_cast<HRESULT>(0x80070057u);
constexpr HRESULT E_NOTIMPL = static_cast<HRESULT>(0x80004001u);
constexpr bool SUCCEEDED(HRESULT value) { return value >= 0; }
constexpr bool FAILED(HRESULT value) { return value < 0; }
constexpr BOOL TRUE = 1, FALSE = 0;
// Key tokens consumed by the legacy UI; SDL translates input to these values.
constexpr int VK_BACK = 8, VK_TAB = 9, VK_RETURN = 13;
constexpr int VK_PRIOR = 33, VK_NEXT = 34, VK_END = 35, VK_HOME = 36;
constexpr int VK_LEFT = 37, VK_UP = 38, VK_RIGHT = 39, VK_DOWN = 40;
constexpr int VK_DELETE = 46;
#define __stdcall
#define __fastcall
#define WINAPI
#define APIENTRY
#define __declspec(value)
inline void OutputDebugStringA(const char* text) { std::fputs(text, stderr); }
inline void OutputDebugString(const char* text) { OutputDebugStringA(text); }
inline DWORD GetTickCount() {
  return static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
}
inline void Sleep(DWORD milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

inline double _wtof(const wchar_t* value) { return std::wcstod(value, nullptr); }
#pragma pack(push, 1)
struct BITMAPFILEHEADER { WORD bfType; DWORD bfSize; WORD bfReserved1, bfReserved2; DWORD bfOffBits; };
struct BITMAPINFOHEADER { DWORD biSize; LONG biWidth, biHeight; WORD biPlanes, biBitCount; DWORD biCompression, biSizeImage; LONG biXPelsPerMeter, biYPelsPerMeter; DWORD biClrUsed, biClrImportant; };
#pragma pack(pop)
constexpr DWORD BI_RGB = 0;
