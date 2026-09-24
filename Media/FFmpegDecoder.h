#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace S2Media {

enum class StreamType { Video, Audio };

struct VideoFrame {
  int width = 0;
  int height = 0;
  std::int64_t timeMs = 0;
  // BGRA8, tightly packed. This matches the game's 32-bit texture path.
  std::vector<std::uint8_t> pixels;
};

struct AudioFrame {
  int sampleRate = 0;
  int channels = 2;
  std::int64_t timeMs = 0;
  // Interleaved stereo float32 PCM for the miniaudio output path.
  std::vector<float> samples;
};

class FFmpegDecoder {
public:
  FFmpegDecoder();
  ~FFmpegDecoder();
  FFmpegDecoder(const FFmpegDecoder&) = delete;
  FFmpegDecoder& operator=(const FFmpegDecoder&) = delete;

  bool Open(const std::string& path, StreamType type, std::string* error = nullptr);
  void Close();
  bool NextVideo(VideoFrame* output);
  bool NextAudio(AudioFrame* output);
  bool SeekMs(std::int64_t timeMs);
  std::int64_t DurationMs() const;
  int VideoWidth() const;
  int VideoHeight() const;
  int VideoFrameRateNumerator() const;
  int VideoFrameRateDenominator() const;
  std::int64_t VideoFrameCount() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};

} // namespace S2Media
