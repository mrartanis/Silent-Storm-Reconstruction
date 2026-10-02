#include "../Main/ChapterTravel.h"
#include <cstdio>
#include <cstdint>
#include <initializer_list>
#include <cmath>

static std::uint32_t Travel(std::uint32_t frame) {
  std::uint32_t last = 0, steps = 0;
  for (std::uint32_t now = frame; now < 1000; now += frame)
    steps += S2Campaign::TravelSteps(now, last);
  return steps + S2Campaign::TravelSteps(1000, last);
}
int main() {
  for (const auto frame : {1u, 16u, 33u, 50u, 100u, 1000u}) {
    const auto steps = Travel(frame);
    std::printf("duration_ms=1000 frame_ms=%u steps=%u expected=60\n", frame, steps);
    if (steps != 60) return 1;
  }
  std::uint32_t last = 100;
  if (S2Campaign::TravelSteps(100, last) || S2Campaign::TravelSteps(116, last)) return 2;
  if (S2Campaign::TravelSteps(117, last) != 1 || last != 117) return 3;
  if (S2Campaign::TravelSteps(5, last) || last != 5) return 4;
  if (S2Campaign::TravelSteps(55, last) != 3) return 5;
  // Run the actual chapter-map step and arrival predicate on the same route,
  // including a mixed sequence of short frames and pauses. Check position at
  // equal game time, not merely the number of calls to the clock helper.
  const std::initializer_list<std::uint32_t> partitions[] = {
    {1}, {16}, {33}, {50}, {1000}, {1,33,7,117,250,16,80,5}
  };
  for (const auto& pattern : partitions) {
    float x = 10, y = 20;
    std::uint32_t now = 123, clock = now;
    auto frame = pattern.begin();
    for (std::uint32_t checkpoint = 1123; checkpoint <= 7123; checkpoint += 1000) {
      while (now < checkpoint) {
        auto delta = *frame++;
        if (frame == pattern.end()) frame = pattern.begin();
        now += delta < checkpoint-now ? delta : checkpoint-now;
        auto steps = S2Campaign::TravelSteps(now, clock);
        while (steps-- > 0 && S2Campaign::AdvanceTravelStep(x,y,410,220,1,.5f)) {}
      }
      const float expectedSteps = checkpoint < 7123 ? float((checkpoint-123)*60/1000) : 400;
      const float expectedX = 10 + expectedSteps, expectedY = 20 + expectedSteps*.5f;
      std::printf("route time_ms=%u position=%.3f,%.3f expected=%.3f,%.3f\n",
                  checkpoint-123,x,y,expectedX,expectedY);
      if (std::fabs(x-expectedX) > .0001f || std::fabs(y-expectedY) > .0001f) return 6;
    }
    if (S2Campaign::AdvanceTravelStep(x,y,410,220,1,.5f)) return 7;
  }
  return 0;
}
