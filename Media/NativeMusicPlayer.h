#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace S2Media {

// The game's .wav music assets contain Vorbis, so FFmpeg decodes them and
// miniaudio owns playback, mixing, volume, looping and seek state.
class NativeMusicPlayer {
public:
  NativeMusicPlayer();
  ~NativeMusicPlayer();
  NativeMusicPlayer(const NativeMusicPlayer&) = delete;
  NativeMusicPlayer& operator=(const NativeMusicPlayer&) = delete;

  bool Init(bool headless = false);
  bool Play(const std::string& path, bool loop = false,
            std::int64_t startMs = 0);
  void Stop();
  void SetPaused(bool paused);
  void SetVolume(float volume);
  bool Seek(std::int64_t timeMs);
  bool IsPlaying() const;
  std::int64_t TimeMs() const;
  // Offline mixer check; only valid when Init(true) was used.
  bool RenderForTest(float* stereoSamples, std::size_t frameCount);

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};

} // namespace S2Media
