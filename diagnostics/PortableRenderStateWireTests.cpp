#include "../FileIO/PortableRenderStateWire.h"

#include <cstdint>
#include <cstring>

template<std::size_t N>
bool CheckFloatRecord() {
  float source[N] = {};
  for (std::size_t i = 0; i < N; ++i) source[i] = static_cast<float>(i) - 2.0f;
  std::uint8_t wire[N * 4] = {};
  float loaded[N] = {};
  return S2FileIO::EncodeRenderFloatFields<N>(source, wire, sizeof(wire)) &&
         S2FileIO::DecodeRenderFloatFields<N>(wire, sizeof(wire), loaded) &&
         std::memcmp(source, loaded, sizeof(source)) == 0 &&
         wire[0] == 0 && wire[1] == 0 && wire[2] == 0 && wire[3] == 0xc0 &&
         !S2FileIO::DecodeRenderFloatFields<N>(wire, sizeof(wire) - 1, loaded) &&
         !S2FileIO::EncodeRenderFloatFields<N>(source, wire, sizeof(wire) - 1);
}

int main() {
  if (!CheckFloatRecord<8>() || !CheckFloatRecord<16>() ||
      !CheckFloatRecord<24>()) return 1;
  S2FileIO::RenderDirectionalFields value{{1, 2, 3}, {4, 5, 6}, true};
  std::uint8_t wire[28] = {};
  if (!S2FileIO::EncodeRenderDirectional(value, wire, sizeof(wire)) ||
      wire[24] != 1 || wire[25] != 0 || wire[26] != 0 || wire[27] != 0)
    return 2;
  S2FileIO::RenderDirectionalFields loaded{};
  wire[24] = 0xff;
  wire[25] = 0xaa;
  if (!S2FileIO::DecodeRenderDirectional(wire, sizeof(wire), &loaded) ||
      !loaded.rendered || std::memcmp(loaded.color, value.color, 12) != 0 ||
      std::memcmp(loaded.direction, value.direction, 12) != 0 ||
      S2FileIO::DecodeRenderDirectional(wire, 27, &loaded) ||
      S2FileIO::EncodeRenderDirectional(value, wire, 27) ||
      S2FileIO::DecodeRenderDirectional(nullptr, 28, &loaded) ||
      S2FileIO::DecodeRenderDirectional(wire, 28, nullptr) ||
      S2FileIO::EncodeRenderDirectional(value, nullptr, 28)) return 3;
  return 0;
}
