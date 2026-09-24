#include "../Media/NativeMusicPlayer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

int main(int argc, char** argv)
{
  if (argc != 2) {
    std::fprintf(stderr, "usage: MusicMixerProbe <original music file>\n");
    return 2;
  }
  S2Media::NativeMusicPlayer player;
  if (!player.Init(true) || !player.Play(argv[1])) {
    std::fprintf(stderr, "cannot initialize/decode music\n");
    return 1;
  }
  std::vector<float> samples(4096 * 2);
  if (!player.RenderForTest(samples.data(), 4096)) return 1;
  float peak = 0.0f;
  for (float sample : samples) peak = std::max(peak, std::fabs(sample));
  if (!(peak > 0.0001f)) return 1;
  const std::int64_t playedMs = player.TimeMs();
  player.SetPaused(true);
  std::vector<float> muted(samples.size());
  if (!player.RenderForTest(muted.data(), 4096)) return 1;
  float pausedPeak = 0.0f;
  for (float value : muted) pausedPeak = std::max(pausedPeak, std::fabs(value));
  if (pausedPeak > 0.000001f || player.TimeMs() != playedMs) {
    std::fprintf(stderr, "pause failed: peak=%f, cursor=%lld vs %lld\n",
        pausedPeak, static_cast<long long>(player.TimeMs()),
        static_cast<long long>(playedMs));
    return 1;
  }
  player.SetPaused(false);
  if (!player.Seek(0) || player.TimeMs() != 0) return 1;
  S2Media::NativeMusicPlayer halfVolumePlayer;
  if (!halfVolumePlayer.Init(true) || !halfVolumePlayer.Play(argv[1])) return 1;
  halfVolumePlayer.SetVolume(0.5f);
  std::vector<float> half(samples.size());
  if (!halfVolumePlayer.RenderForTest(half.data(), 4096)) return 1;
  float largestDifference = 0.0f;
  for (std::size_t i = 0; i < samples.size(); ++i)
    largestDifference = std::max(largestDifference,
        std::fabs(half[i] - samples[i] * 0.5f));
  if (largestDifference > 0.0001f) {
    std::fprintf(stderr, "seek/volume failed: max difference=%f\n",
                 largestDifference);
    return 1;
  }
  std::printf("miniaudio mixed audio, peak=%f, cursor=%lld ms; pause, seek and volume OK\n",
              peak, static_cast<long long>(playedMs));
  return 0;
}
