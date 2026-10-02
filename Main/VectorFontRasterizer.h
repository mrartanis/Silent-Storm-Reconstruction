#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace S2Fonts {
struct Glyph {
  std::uint16_t code;
  int x, y, width, height, bearing, advance;
};
struct Kern { std::uint16_t previous, current; int adjustment; };
struct Atlas {
  int width = 0, height = 0, lineHeight = 0, averageWidth = 0, maxWidth = 0;
  std::vector<Glyph> glyphs;
  std::vector<Kern> kerns;
  std::vector<unsigned char> rgba;
};
std::string FamilyFile(const std::string& family);
bool Rasterize(const std::string& path, int pixels, Atlas* result, std::string* error);
}
