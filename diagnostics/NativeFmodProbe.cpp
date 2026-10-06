#include <fmod.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

extern "C" int S2FmodRenderForTest(float* output, std::size_t frames);

int main(int argc, char** argv)
{
  if (argc < 2 || !FSOUND_Init(44100, 32, 0)) return 2;
  int checked = 0;
  for (int i = 1; i < argc; ++i) {
    std::ifstream input(argv[i], std::ios::binary);
    if (!input) return 3;
    std::vector<char> bytes((std::istreambuf_iterator<char>(input)),
                             std::istreambuf_iterator<char>());
    FSOUND_SAMPLE* sample = FSOUND_Sample_Load(FSOUND_UNMANAGED,
        bytes.data(), FSOUND_HW3D | FSOUND_LOADMEMORY, 0,
        static_cast<int>(bytes.size()));
    if (!sample || !FSOUND_Sample_GetLength(sample)) {
      std::cerr << "decode failed: " << argv[i] << '\n';
      return 4;
    }
    FSOUND_Sample_SetMinMaxDistance(sample, 1, 100);
    const int loopEnd = std::min<int>(512, FSOUND_Sample_GetLength(sample));
    if (loopEnd <= 1 || !FSOUND_Sample_SetLoopPoints(sample, 0, loopEnd)) return 5;
    const int channel = FSOUND_PlaySoundEx(FSOUND_FREE, sample, nullptr, 1);
    if (channel < 0 || FSOUND_GetFrequency(channel) <= 0) return 5;
    const float position[3] = {1, 0, 0};
    if (!FSOUND_3D_SetAttributes(channel, position, nullptr) ||
        !FSOUND_SetVolume(channel, 128) ||
        FSOUND_GetVolume(channel) != 128 ||
        !FSOUND_SetCurrentPosition(channel, 0) ||
        !FSOUND_SetLoopMode(channel, FSOUND_LOOP_NORMAL) ||
        FSOUND_GetLoopMode(channel) != FSOUND_LOOP_NORMAL ||
        !FSOUND_SetPaused(channel, 0)) return 6;
    std::vector<float> output(20000);
    if (!S2FmodRenderForTest(output.data(), output.size() / 2)) return 7;
    float peak = 0;
    for (float value : output) peak = std::max(peak, std::abs(value));
    const unsigned int cursor = FSOUND_GetCurrentPosition(channel);
    float tailPeak = 0;
    for (std::size_t n = output.size() / 2; n < output.size(); ++n)
      tailPeak = std::max(tailPeak, std::abs(output[n]));
    if (peak <= 0.0000001f || tailPeak <= 0.0000001f ||
        cursor > static_cast<unsigned int>(loopEnd) ||
        !FSOUND_SetPaused(channel, 1) || !FSOUND_IsPlaying(channel)) {
      std::cerr << "3D playback failed: peak=" << peak << " tail=" << tailPeak
                << " cursor=" << cursor
                << " loopEnd=" << loopEnd << " file=" << argv[i] << '\n';
      return 8;
    }
    if (i == 1) {
      FSOUND_SetSFXMasterVolume(0);
      FSOUND_SetPaused(channel, 0);
      std::fill(output.begin(), output.end(), 0.0f);
      if (!S2FmodRenderForTest(output.data(), output.size() / 2)) return 12;
      float mutedPeak = 0;
      for (float value : output) mutedPeak = std::max(mutedPeak, std::abs(value));
      if (mutedPeak > 0.000001f) return 13;
      FSOUND_SetSFXMasterVolume(128);
      FSOUND_SetCurrentPosition(channel,0);
      if (!S2FmodRenderForTest(output.data(),output.size()/2)) return 14;
      float halfPeak=0;for(float value:output)halfPeak=std::max(halfPeak,std::abs(value));
      FSOUND_SetSFXMasterVolume(255);
      FSOUND_SetCurrentPosition(channel,0);
      if (!S2FmodRenderForTest(output.data(),output.size()/2)) return 15;
      float restoredPeak=0;for(float value:output)restoredPeak=std::max(restoredPeak,std::abs(value));
      const float ratio=halfPeak/restoredPeak;
      if(restoredPeak<=0.0000001f || ratio<0.45f || ratio>0.55f)return 16;
      std::printf("Live master gain: muted=%g half/full=%g restored=%g\n",mutedPeak,ratio,restoredPeak);
    }
    FSOUND_StopSound(channel);
    FSOUND_Sample_Free(sample);
    if (i == 1) {
      FSOUND_SAMPLE* sample2d = FSOUND_Sample_Load(FSOUND_UNMANAGED,
          bytes.data(), FSOUND_2D | FSOUND_LOADMEMORY, 0,
          static_cast<int>(bytes.size()));
      if (!sample2d) return 9;
      const int channel2d = FSOUND_PlaySoundEx(FSOUND_FREE, sample2d, nullptr, 0);
      if (channel2d < 0 || !S2FmodRenderForTest(output.data(), output.size() / 2))
        return 10;
      float peak2d = 0;
      for (float value : output) peak2d = std::max(peak2d, std::abs(value));
      if (peak2d <= 0.00001f) return 11;
      FSOUND_StopSound(channel2d);
      FSOUND_Sample_Free(sample2d);
    }
    ++checked;
  }
  FSOUND_Close();
  std::cout << "decoded and played " << checked << " samples\n";
  return 0;
}
