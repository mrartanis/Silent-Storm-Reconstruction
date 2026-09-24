#include "FFmpegDecoder.h"
#include "NativeMusicPlayer.h"
#include <bink.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

namespace {
using Clock = std::chrono::steady_clock;

struct Movie {
  BINK header{}; // The game's Bink header reads use this exact leading layout.
  S2Media::FFmpegDecoder video;
  S2Media::NativeMusicPlayer audio;
  S2Media::VideoFrame decoded;
  Clock::time_point started;
  Clock::time_point pausedAt;
  bool paused = false;
  bool hasFrame = false;
  bool prefetched = false;
};

Movie* State(HBINK handle) { return reinterpret_cast<Movie*>(handle); }

std::int64_t FrameTimeMs(const Movie* movie, U32 frame)
{
  return static_cast<std::int64_t>(frame - 1) * 1000 *
         movie->header.FrameRateDiv / movie->header.FrameRate;
}
}

extern "C" {

__declspec(dllexport) HBINK __stdcall BinkOpen(const char* name, U32)
{
  if (!name) return nullptr;
  std::unique_ptr<Movie> movie(new Movie);
  if (!movie->video.Open(name, S2Media::StreamType::Video)) return nullptr;
  movie->header.Width = movie->video.VideoWidth();
  movie->header.Height = movie->video.VideoHeight();
  movie->header.FrameRate = movie->video.VideoFrameRateNumerator();
  movie->header.FrameRateDiv = movie->video.VideoFrameRateDenominator();
  movie->header.Frames = static_cast<U32>(movie->video.VideoFrameCount());
  if (!movie->header.Width || !movie->header.Height ||
      !movie->header.FrameRate || !movie->header.FrameRateDiv ||
      !movie->header.Frames) return nullptr;
  movie->header.FrameNum = 1;
  movie->started = Clock::now();
  // A movie without audio remains playable; FFmpeg selects its first audio
  // stream when present, while miniaudio owns the output device.
  if (movie->audio.Init()) movie->audio.Play(name);
  return &movie.release()->header;
}

__declspec(dllexport) void __stdcall BinkClose(HBINK handle)
{
  delete State(handle);
}

__declspec(dllexport) S32 __stdcall BinkWait(HBINK handle)
{
  if (!handle) return 1;
  Movie* movie = State(handle);
  if (movie->paused) return 1;
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      Clock::now() - movie->started).count();
  return elapsed >= FrameTimeMs(movie, movie->header.FrameNum) ? 0 : 1;
}

__declspec(dllexport) S32 __stdcall BinkDoFrame(HBINK handle)
{
  if (!handle) return 0;
  Movie* movie = State(handle);
  if (movie->prefetched) {
    movie->prefetched = false;
    movie->hasFrame = true;
    return 1;
  }
  movie->hasFrame = movie->video.NextVideo(&movie->decoded);
  return movie->hasFrame ? 1 : 0;
}

__declspec(dllexport) void __stdcall BinkNextFrame(HBINK handle)
{
  if (!handle) return;
  Movie* movie = State(handle);
  movie->header.LastFrameNum = movie->header.FrameNum;
  if (++movie->header.FrameNum > movie->header.Frames) {
    movie->header.FrameNum = 1;
    movie->video.SeekMs(0);
    movie->audio.Seek(0);
    movie->started = Clock::now();
    movie->prefetched = false;
  }
}

__declspec(dllexport) void __stdcall BinkGoto(HBINK handle, U32 frame, S32)
{
  if (!handle) return;
  Movie* movie = State(handle);
  frame = std::max<U32>(1, std::min(frame, movie->header.Frames));
  const std::int64_t timeMs = FrameTimeMs(movie, frame);
  if (movie->video.SeekMs(timeMs)) {
    S2Media::VideoFrame sought;
    bool found = false;
    while (movie->video.NextVideo(&sought)) {
      if (sought.timeMs >= timeMs) {
        movie->decoded = std::move(sought);
        found = true;
        break;
      }
    }
    if (!found) return;
    movie->header.FrameNum = frame;
    movie->audio.Seek(timeMs);
    movie->started = Clock::now() - std::chrono::milliseconds(timeMs);
    movie->hasFrame = false;
    movie->prefetched = true;
  }
}

__declspec(dllexport) S32 __stdcall BinkPause(HBINK handle, S32 pause)
{
  if (!handle) return 0;
  Movie* movie = State(handle);
  if (pause && !movie->paused) {
    movie->pausedAt = Clock::now();
    movie->paused = true;
    movie->audio.SetPaused(true);
  } else if (!pause && movie->paused) {
    movie->started += Clock::now() - movie->pausedAt;
    movie->paused = false;
    movie->audio.SetPaused(false);
  }
  return 1;
}

__declspec(dllexport) void __stdcall BinkSetVolume(HBINK handle, U32, S32 volume)
{
  if (handle) State(handle)->audio.SetVolume(
      std::max(0.0f, std::min(1.0f, volume / 32768.0f)));
}

__declspec(dllexport) S32 __stdcall BinkCopyToBuffer(HBINK handle, void* dest,
    S32 pitch, U32 destheight, U32 destx, U32 desty, U32 flags)
{
  if (!handle || !dest || pitch <= 0) return 0;
  const Movie* movie = State(handle);
  if (!movie->hasFrame || destheight < desty + movie->header.Height) return 0;
  const bool sixteenBit = (flags & 0xf) == 8;
  const int bytesPerPixel = sixteenBit ? 2 : 4;
  if (pitch < static_cast<S32>((destx + movie->header.Width) * bytesPerPixel)) return 0;
  for (U32 y = 0; y < movie->header.Height; ++y) {
    const std::uint8_t* src = movie->decoded.pixels.data() +
                              static_cast<std::size_t>(y) * movie->header.Width * 4;
    std::uint8_t* row = static_cast<std::uint8_t*>(dest) +
                        static_cast<std::size_t>(desty + y) * pitch +
                        static_cast<std::size_t>(destx) * bytesPerPixel;
    if (!sixteenBit) {
      std::memcpy(row, src, static_cast<std::size_t>(movie->header.Width) * 4);
    } else {
      for (U32 x = 0; x < movie->header.Width; ++x) {
        const std::uint16_t pixel = (src[x*4+3] >= 128 ? 0x8000 : 0) |
            ((src[x*4+2] >> 3) << 10) | ((src[x*4+1] >> 3) << 5) |
            (src[x*4] >> 3);
        std::memcpy(row + x*2, &pixel, sizeof(pixel));
      }
    }
  }
  return 1;
}

__declspec(dllexport) S32 __stdcall BinkOpenDirectSound(U32) { return 1; }
__declspec(dllexport) S32 __stdcall BinkSetSoundSystem(BINKSNDSYSOPEN, U32) { return 1; }

} // extern "C"
