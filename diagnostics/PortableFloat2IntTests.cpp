#include "../Misc/PortableFloat2Int.h"

#include <cfenv>
#include <cmath>
#include <cstdint>
#include <limits>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <xmmintrin.h>
#define S2_TEST_X86_CONVERSION 1
#endif

int main() {
  const int originalMode = std::fegetround();
  if (originalMode < 0) return 1;
  const int modes[] = {FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD};
  const float inputs[] = {-2.5f, -1.5f, -0.5f, -0.0f, 0.0f,
                          0.5f, 1.5f, 2.5f, 2147483520.0f,
                          2147483648.0f, -2147483648.0f, -2147483904.0f};
  const std::int32_t ties[][6] = {
      {-2, -2, 0, 0, 2, 2},
      {-2, -1, 0, 0, 1, 2},
      {-2, -1, 0, 1, 2, 3},
      {-3, -2, -1, 0, 1, 2}};
  for (unsigned mode = 0; mode < 4; ++mode) {
    if (std::fesetround(modes[mode]) != 0) return 2;
    for (float input : inputs) {
      volatile float runtimeInput = input;
      const std::int32_t actual = S2Math::Float2IntWithCurrentRounding(runtimeInput);
#if defined(S2_TEST_X86_CONVERSION)
      const std::int32_t reference = _mm_cvtss_si32(_mm_set_ss(runtimeInput));
      if (actual != reference) return 3;
#endif
    }
    for (unsigned i = 0; i < 3; ++i) {
      volatile float negative = inputs[i];
      volatile float positive = inputs[i + 5];
      if (S2Math::Float2IntWithCurrentRounding(negative) != ties[mode][i] ||
          S2Math::Float2IntWithCurrentRounding(positive) != ties[mode][i + 3])
        return 4;
    }
    const std::int32_t edgeResults[] = {
        2147483520, INT32_MIN, INT32_MIN, INT32_MIN};
    for (unsigned i = 0; i < 4; ++i) {
      volatile float edge = inputs[i + 8];
      if (S2Math::Float2IntWithCurrentRounding(edge) != edgeResults[i]) return 7;
    }
  }
  if (std::fesetround(originalMode) != 0) return 5;
  if (S2Math::Float2IntWithCurrentRounding(std::numeric_limits<float>::infinity()) !=
          std::numeric_limits<std::int32_t>::min() ||
      S2Math::Float2IntWithCurrentRounding(std::numeric_limits<float>::quiet_NaN()) !=
          std::numeric_limits<std::int32_t>::min())
    return 6;
  return 0;
}
