#include "VectorFontRasterizer.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <cctype>
#include <cstring>

namespace S2Fonts {
std::string FamilyFile(const std::string& family) {
  std::string key = family;
  for (char& c : key) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  if (key == "system" || key == "arial" || key == "liberation sans") return "LiberationSans-Regular.ttf";
  if (key == "times" || key == "times new roman" || key == "liberation serif") return "LiberationSerif-Regular.ttf";
  if (key == "courier" || key == "courier new" || key == "liberation mono") return "LiberationMono-Regular.ttf";
  return {};
}
bool Rasterize(const std::string& path, int pixels, Atlas* result, std::string* error) {
  if (!result || pixels < 1 || pixels > 512) { if (error) *error = "invalid font size"; return false; }
  FT_Library library = nullptr;
  FT_Face face = nullptr;
  if (FT_Init_FreeType(&library)) { if (error) *error = "FreeType initialization failed"; return false; }
  struct Cleanup { FT_Library lib; FT_Face& face; ~Cleanup() { if (face) FT_Done_Face(face); FT_Done_FreeType(lib); } } cleanup{library, face};
  if (FT_New_Face(library, path.c_str(), 0, &face) || FT_Set_Pixel_Sizes(face, 0, pixels)) {
    if (error) *error = "cannot load vector font: " + path; return false;
  }
  Atlas atlas;
  const int ascent = (face->size->metrics.ascender + 63) >> 6;
  atlas.lineHeight = std::max(pixels, static_cast<int>((face->size->metrics.height + 63) >> 6));
  atlas.width = pixels > 64 ? 4096 : 2048;
  int x = 1, y = 1, rowHeight = 0;
  struct Bitmap { Glyph glyph; std::vector<unsigned char> bytes; };
  std::vector<Bitmap> bitmaps;
  std::vector<FT_UInt> indices;
  for (unsigned code = 32; code <= 0xfffd; ++code) {
    if (!(code <= 0x2ff || (code >= 0x400 && code <= 0x52f) ||
          (code >= 0x2000 && code <= 0x206f) || code == 0x20ac ||
          (code >= 0x2500 && code <= 0x257f) || code == 0xfffd)) continue;
    const FT_UInt index = FT_Get_Char_Index(face, code);
    if (!index || FT_Load_Glyph(face, index, FT_LOAD_DEFAULT) || FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) continue;
    const auto& src = face->glyph->bitmap;
    const int w = static_cast<int>(src.width), h = atlas.lineHeight;
    if (w + 2 > atlas.width) { if (error) *error = "glyph too large"; return false; }
    if (x + w + 1 > atlas.width) { x = 1; y += rowHeight + 1; rowHeight = 0; }
    Bitmap bitmap;
    bitmap.glyph = {static_cast<std::uint16_t>(code), x, y, w, h, face->glyph->bitmap_left,
                   static_cast<int>((face->glyph->advance.x + 32) >> 6)};
    bitmap.bytes.assign(w * h, 0);
    const int top = ascent - face->glyph->bitmap_top;
    for (int sy = 0; sy < static_cast<int>(src.rows); ++sy) {
      const int dy = top + sy;
      if (dy < 0 || dy >= h) continue;
      const auto* row = src.buffer + (src.pitch >= 0 ? sy * src.pitch : (src.rows - 1 - sy) * -src.pitch);
      for (int sx = 0; sx < w; ++sx) bitmap.bytes[dy * w + sx] = row[sx];
    }
    if (code == 'x') atlas.averageWidth = bitmap.glyph.advance;
    atlas.maxWidth = std::max(atlas.maxWidth, bitmap.glyph.advance);
    indices.push_back(index); bitmaps.push_back(std::move(bitmap));
    x += w + 2; rowHeight = std::max(rowHeight, h);
  }
  atlas.height = 1;
  while (atlas.height < y + rowHeight + 1) atlas.height *= 2;
  if (atlas.height > 16384) { if (error) *error = "font atlas exceeds texture limit"; return false; }
  atlas.rgba.assign(static_cast<std::size_t>(atlas.width) * atlas.height * 4, 0);
  for (const auto& b : bitmaps) {
    atlas.glyphs.push_back(b.glyph);
    for (int sy = 0; sy < b.glyph.height; ++sy) for (int sx = 0; sx < b.glyph.width; ++sx) {
      const auto alpha = b.bytes[sy * b.glyph.width + sx];
      auto* dst = &atlas.rgba[((b.glyph.y + sy) * atlas.width + b.glyph.x + sx) * 4];
      // UI uses premultiplied alpha; white glyph pixels must be premultiplied too.
      dst[0] = dst[1] = dst[2] = dst[3] = alpha;
    }
  }
  if (FT_HAS_KERNING(face)) for (std::size_t a = 0; a < indices.size(); ++a) for (std::size_t b = 0; b < indices.size(); ++b) {
    FT_Vector delta{};
    if (!FT_Get_Kerning(face, indices[a], indices[b], FT_KERNING_DEFAULT, &delta) && delta.x)
      atlas.kerns.push_back({atlas.glyphs[a].code, atlas.glyphs[b].code, static_cast<int>(delta.x / 64)});
  }
  if (atlas.glyphs.empty()) { if (error) *error = "font has no supported glyphs"; return false; }
  *result = std::move(atlas); return true;
}
}
