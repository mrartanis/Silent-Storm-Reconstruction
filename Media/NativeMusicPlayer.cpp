#include "NativeMusicPlayer.h"
#include "FFmpegDecoder.h"
#include <miniaudio.h>

#include <algorithm>
#include <vector>

namespace S2Media {

struct NativeMusicPlayer::Impl {
  ma_engine engine{};
  ma_audio_buffer buffer{};
  ma_sound sound{};
  std::vector<float> pcm;
  int sampleRate = 0;
  bool initialized = false;
  bool bufferReady = false;
  bool soundReady = false;
  bool headless = false;
  bool paused = false;

  void Stop()
  {
    if (soundReady) {
      ma_sound_uninit(&sound);
      soundReady = false;
    }
    if (bufferReady) {
      ma_audio_buffer_uninit(&buffer);
      bufferReady = false;
    }
    pcm.clear();
    sampleRate = 0;
    paused = false;
  }

  ~Impl()
  {
    Stop();
    if (initialized) ma_engine_uninit(&engine);
  }
};

NativeMusicPlayer::NativeMusicPlayer(): impl(new Impl) {}
NativeMusicPlayer::~NativeMusicPlayer() = default;

bool NativeMusicPlayer::Init(bool headless)
{
  if (impl->initialized) return impl->headless == headless;
  ma_engine_config config = ma_engine_config_init();
  if (headless) {
    config.noDevice = MA_TRUE;
    config.channels = 2;
    config.sampleRate = 44100;
  }
  impl->initialized = ma_engine_init(&config, &impl->engine) == MA_SUCCESS;
  impl->headless = headless;
  return impl->initialized;
}

bool NativeMusicPlayer::Play(const std::string& path, bool loop,
                             std::int64_t startMs)
{
  if (!impl->initialized || startMs < 0) return false;
  Stop();
  FFmpegDecoder decoder;
  if (!decoder.Open(path, StreamType::Audio)) return false;
  AudioFrame frame;
  while (decoder.NextAudio(&frame)) {
    if (frame.channels != 2 || frame.sampleRate <= 0 ||
        (impl->sampleRate && impl->sampleRate != frame.sampleRate))
      return false;
    impl->sampleRate = frame.sampleRate;
    impl->pcm.insert(impl->pcm.end(), frame.samples.begin(), frame.samples.end());
  }
  if (!impl->sampleRate || impl->pcm.empty()) return false;
  const ma_uint64 frameCount = static_cast<ma_uint64>(impl->pcm.size() / 2);
  ma_audio_buffer_config config = ma_audio_buffer_config_init(
      ma_format_f32, 2, frameCount, impl->pcm.data(), nullptr);
  config.sampleRate = static_cast<ma_uint32>(impl->sampleRate);
  if (ma_audio_buffer_init(&config, &impl->buffer) != MA_SUCCESS) return false;
  impl->bufferReady = true;
  if (ma_sound_init_from_data_source(&impl->engine, &impl->buffer, 0,
                                     nullptr, &impl->sound) != MA_SUCCESS)
    return false;
  impl->soundReady = true;
  ma_sound_set_looping(&impl->sound, loop ? MA_TRUE : MA_FALSE);
  const ma_uint64 firstFrame = static_cast<ma_uint64>(startMs) * impl->sampleRate / 1000;
  if (firstFrame >= frameCount || ma_sound_seek_to_pcm_frame(&impl->sound, firstFrame) != MA_SUCCESS)
    return false;
  return ma_sound_start(&impl->sound) == MA_SUCCESS;
}

void NativeMusicPlayer::Stop() { impl->Stop(); }

void NativeMusicPlayer::SetPaused(bool paused)
{
  if (!impl->soundReady || paused == impl->paused) return;
  if (paused) ma_sound_stop(&impl->sound);
  else ma_sound_start(&impl->sound);
  impl->paused = paused;
}

void NativeMusicPlayer::SetVolume(float volume)
{
  if (impl->soundReady) ma_sound_set_volume(&impl->sound,
      std::max(0.0f, std::min(1.0f, volume)));
}

bool NativeMusicPlayer::Seek(std::int64_t timeMs)
{
  if (!impl->soundReady || timeMs < 0) return false;
  const ma_uint64 frame = static_cast<ma_uint64>(timeMs) * impl->sampleRate / 1000;
  return ma_sound_seek_to_pcm_frame(&impl->sound, frame) == MA_SUCCESS;
}

bool NativeMusicPlayer::IsPlaying() const
{
  return impl->soundReady && !impl->paused &&
         ma_sound_is_playing(&impl->sound) == MA_TRUE;
}

std::int64_t NativeMusicPlayer::TimeMs() const
{
  if (!impl->soundReady || !impl->sampleRate) return -1;
  ma_uint64 cursor = 0;
  if (ma_sound_get_cursor_in_pcm_frames(&impl->sound, &cursor) != MA_SUCCESS)
    return -1;
  return static_cast<std::int64_t>(cursor * 1000 / impl->sampleRate);
}

bool NativeMusicPlayer::RenderForTest(float* stereoSamples,
                                      std::size_t frameCount)
{
  if (!impl->initialized || !impl->headless || !stereoSamples) return false;
  return ma_engine_read_pcm_frames(&impl->engine, stereoSamples,
           static_cast<ma_uint64>(frameCount), nullptr) == MA_SUCCESS;
}

} // namespace S2Media
