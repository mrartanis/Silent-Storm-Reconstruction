#include "FFmpegDecoder.h"
#include <fmod.h>
#include <miniaudio.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <vector>

// Game-facing subset of FMOD 3. The x86 build still links the original DLL.
// Samples are Ogg/Vorbis blobs from Sounds.res; FFmpeg decodes them in memory.
struct FSOUND_SAMPLE {
  std::vector<float> pcm;
  int rate = 0;
  unsigned int mode = FSOUND_LOOP_OFF;
  unsigned int loopBegin = 0;
  unsigned int loopEnd = 0;
  float minDistance = 1;
  float maxDistance = 10000;
  int priority = 128;
};
struct FSOUND_STREAM {};

namespace {
struct Channel {
  FSOUND_SAMPLE* sample = nullptr;
  ma_audio_buffer buffer{};
  ma_sound sound{};
  bool bufferReady = false;
  bool soundReady = false;
  bool paused = false;
  int volume = 255;
  unsigned int loopMode = FSOUND_LOOP_OFF;
  ~Channel() {
    if (soundReady) ma_sound_uninit(&sound);
    if (bufferReady) ma_audio_buffer_uninit(&buffer);
  }
};

ma_engine engine{};
bool initialized = false;
int masterVolume = 255;
int maxChannels = 32;
int nextChannel = 1;
std::unordered_map<int, std::unique_ptr<Channel>> channels;

Channel* Find(int id) {
  const auto found = channels.find(id);
  return found == channels.end() ? nullptr : found->second.get();
}
float UnitVolume(int value) {
  return std::max(0, std::min(255, value)) / 255.0f;
}
void ApplyVolume(Channel* channel) {
  ma_sound_set_volume(&channel->sound,
      UnitVolume(channel->volume) * UnitVolume(masterVolume));
}
void ApplyLoop(Channel* channel) {
  ma_sound_set_looping(&channel->sound,
      channel->loopMode == FSOUND_LOOP_NORMAL ? MA_TRUE : MA_FALSE);
  const ma_uint64 length = channel->sample->pcm.size() / 2;
  const ma_uint64 end = channel->sample->loopEnd ?
      std::min<ma_uint64>(channel->sample->loopEnd, length) : length;
  if (channel->sample->loopBegin < end)
    ma_data_source_set_loop_point_in_pcm_frames(
        ma_sound_get_data_source(&channel->sound), channel->sample->loopBegin, end);
}
}

extern "C" {
int S2FmodRenderForTest(float* output, std::size_t frames) {
  return initialized && std::getenv("S2_AUDIO_HEADLESS") && output &&
      ma_engine_read_pcm_frames(&engine, output, frames, nullptr) == MA_SUCCESS;
}
signed char __stdcall FSOUND_SetOutput(int) { return 1; }
signed char __stdcall FSOUND_SetDriver(int) { return 1; }
signed char __stdcall FSOUND_SetHWND(void*) { return 1; }
signed char __stdcall FSOUND_Init(int mixrate, int maximum, unsigned int) {
  if (initialized) return 1;
  ma_engine_config config = ma_engine_config_init();
  config.channels = 2;
  config.sampleRate = mixrate > 0 ? mixrate : 44100;
  config.noDevice = std::getenv("S2_AUDIO_HEADLESS") ? MA_TRUE : MA_FALSE;
  if (ma_engine_init(&config, &engine) != MA_SUCCESS) return 0;
  initialized = true;
  maxChannels = maximum > 0 ? maximum : 32;
  return 1;
}
void __stdcall FSOUND_Close(void) {
  channels.clear();
  if (initialized) ma_engine_uninit(&engine);
  initialized = false;
}
void __stdcall FSOUND_Update(void) {
  for (auto it = channels.begin(); it != channels.end(); ) {
    Channel* channel = it->second.get();
    if (!channel->paused && ma_sound_is_playing(&channel->sound) != MA_TRUE)
      it = channels.erase(it);
    else ++it;
  }
}
int __stdcall FSOUND_GetNumDrivers(void) { return 1; }
const char* __stdcall FSOUND_GetDriverName(int) { return "miniaudio"; }
signed char __stdcall FSOUND_GetDriverCaps(int, unsigned int* caps) {
  if (caps) *caps = 0;
  return 1;
}
float __stdcall FSOUND_GetVersion(void) { return FMOD_VERSION; }
int __stdcall FSOUND_GetMixer(void) { return FSOUND_MIXER_QUALITY_FPU; }
void* __stdcall FSOUND_GetOutputHandle(void) { return nullptr; }
int __stdcall FSOUND_GetError(void) { return 0; }
void __stdcall FSOUND_SetSFXMasterVolume(int volume) {
  masterVolume = volume;
  for (auto& item : channels) ApplyVolume(item.second.get());
}
signed char __stdcall FSOUND_SetSpeakerMode(unsigned int) { return 1; }

FSOUND_SAMPLE* __stdcall FSOUND_Sample_Load(int, const char* data,
    unsigned int mode, int offset, int length) {
  if (!initialized || !data || length <= 0 || offset < 0) return nullptr;
  S2Media::FFmpegDecoder decoder;
  if (!decoder.OpenMemory(data + offset, length, S2Media::StreamType::Audio))
    return nullptr;
  std::unique_ptr<FSOUND_SAMPLE> sample(new FSOUND_SAMPLE);
  sample->mode = mode;
  S2Media::AudioFrame frame;
  while (decoder.NextAudio(&frame)) {
    if (frame.channels != 2 || frame.sampleRate <= 0 ||
        (sample->rate && sample->rate != frame.sampleRate)) return nullptr;
    sample->rate = frame.sampleRate;
    sample->pcm.insert(sample->pcm.end(), frame.samples.begin(), frame.samples.end());
  }
  if (!sample->rate || sample->pcm.empty()) return nullptr;
  sample->loopEnd = static_cast<unsigned int>(sample->pcm.size() / 2);
  return sample.release();
}
void __stdcall FSOUND_Sample_Free(FSOUND_SAMPLE* sample) {
  if (!sample) return;
  for (auto it = channels.begin(); it != channels.end(); ) {
    if (it->second->sample == sample) it = channels.erase(it);
    else ++it;
  }
  delete sample;
}
unsigned int __stdcall FSOUND_Sample_GetLength(FSOUND_SAMPLE* sample) {
  return sample ? static_cast<unsigned int>(sample->pcm.size() / 2) : 0;
}
signed char __stdcall FSOUND_Sample_SetMode(FSOUND_SAMPLE* sample, unsigned int mode) {
  if (!sample) return 0;
  // Game's 3D call site passes only a loop flag; FMOD retains the sample's
  // original spatial mode in that case.
  const unsigned int spatial = mode & (FSOUND_2D | FSOUND_HW3D);
  sample->mode = (sample->mode & ~(FSOUND_LOOP_OFF | FSOUND_LOOP_NORMAL |
                   FSOUND_2D | FSOUND_HW3D)) |
                 (spatial ? spatial : (sample->mode & (FSOUND_2D | FSOUND_HW3D))) |
                 (mode & (FSOUND_LOOP_OFF | FSOUND_LOOP_NORMAL));
  return 1;
}
signed char __stdcall FSOUND_Sample_SetDefaults(FSOUND_SAMPLE* sample,
    int, int, int, int priority) {
  if (!sample) return 0;
  if (priority >= 0) sample->priority = priority;
  return 1;
}
signed char __stdcall FSOUND_Sample_SetMinMaxDistance(FSOUND_SAMPLE* sample,
    float minimum, float maximum) {
  if (!sample) return 0;
  sample->minDistance = std::max(0.001f, minimum);
  sample->maxDistance = std::max(sample->minDistance, maximum);
  return 1;
}
signed char __stdcall FSOUND_Sample_SetLoopPoints(FSOUND_SAMPLE* sample,
    int begin, int end) {
  if (!sample || begin < 0 || end <= begin ||
      static_cast<unsigned int>(end) > FSOUND_Sample_GetLength(sample)) return 0;
  sample->loopBegin = begin;
  sample->loopEnd = end;
  return 1;
}

int __stdcall FSOUND_PlaySound(int requested, FSOUND_SAMPLE* sample) {
  return FSOUND_PlaySoundEx(requested, sample, nullptr, 0);
}
int __stdcall FSOUND_PlaySoundEx(int requested, FSOUND_SAMPLE* sample,
    FSOUND_DSPUNIT*, signed char startPaused) {
  if (!initialized || !sample || sample->pcm.empty()) return -1;
  FSOUND_Update();
  if (channels.size() >= static_cast<std::size_t>(maxChannels)) return -1;
  std::unique_ptr<Channel> channel(new Channel);
  channel->sample = sample;
  channel->loopMode = sample->mode & FSOUND_LOOP_NORMAL ?
      FSOUND_LOOP_NORMAL : FSOUND_LOOP_OFF;
  channel->paused = startPaused != 0;
  ma_audio_buffer_config config = ma_audio_buffer_config_init(ma_format_f32, 2,
      sample->pcm.size() / 2, sample->pcm.data(), nullptr);
  config.sampleRate = sample->rate;
  if (ma_audio_buffer_init(&config, &channel->buffer) != MA_SUCCESS) return -1;
  channel->bufferReady = true;
  if (ma_sound_init_from_data_source(&engine, &channel->buffer, 0,
      nullptr, &channel->sound) != MA_SUCCESS) return -1;
  channel->soundReady = true;
  const bool spatial = (sample->mode & FSOUND_HW3D) != 0;
  ma_sound_set_spatialization_enabled(&channel->sound, spatial ? MA_TRUE : MA_FALSE);
  if (spatial) {
    ma_sound_set_attenuation_model(&channel->sound, ma_attenuation_model_inverse);
    ma_sound_set_min_distance(&channel->sound, sample->minDistance);
    ma_sound_set_max_distance(&channel->sound, sample->maxDistance);
  }
  ApplyLoop(channel.get());
  ApplyVolume(channel.get());
  const int id = requested >= 0 ? requested : nextChannel++;
  channels[id] = std::move(channel);
  if (!startPaused && ma_sound_start(&channels[id]->sound) != MA_SUCCESS) {
    channels.erase(id);
    return -1;
  }
  return id;
}
signed char __stdcall FSOUND_StopSound(int id) {
  return channels.erase(id) ? 1 : 0;
}
signed char __stdcall FSOUND_IsPlaying(int id) {
  Channel* channel = Find(id);
  // FMOD treats a paused live channel as still allocated/playing. The engine's
  // update loop otherwise drops it before the menu can resume the sound.
  return channel && (channel->paused ||
      ma_sound_is_playing(&channel->sound) == MA_TRUE);
}
signed char __stdcall FSOUND_SetVolume(int id, int volume) {
  Channel* channel = Find(id);
  if (!channel) return 0;
  channel->volume = volume;
  ApplyVolume(channel);
  return 1;
}
signed char __stdcall FSOUND_SetVolumeAbsolute(int id, int volume) {
  return FSOUND_SetVolume(id, volume);
}
int __stdcall FSOUND_GetVolume(int id) {
  Channel* channel = Find(id);
  return channel ? channel->volume : 0;
}
signed char __stdcall FSOUND_SetPaused(int id, signed char paused) {
  Channel* channel = Find(id);
  if (!channel) return 0;
  channel->paused = paused != 0;
  return (paused ? ma_sound_stop(&channel->sound) :
      ma_sound_start(&channel->sound)) == MA_SUCCESS;
}
signed char __stdcall FSOUND_SetPan(int, int) { return 1; }
signed char __stdcall FSOUND_SetCurrentPosition(int id, unsigned int offset) {
  Channel* channel = Find(id);
  return channel && ma_sound_seek_to_pcm_frame(&channel->sound, offset) == MA_SUCCESS;
}
int __stdcall FSOUND_GetFrequency(int id) {
  Channel* channel = Find(id);
  return channel ? channel->sample->rate : 0;
}
unsigned int __stdcall FSOUND_GetCurrentPosition(int id) {
  Channel* channel = Find(id);
  ma_uint64 cursor = 0;
  if (!channel || ma_sound_get_cursor_in_pcm_frames(&channel->sound, &cursor) != MA_SUCCESS)
    return 0;
  return static_cast<unsigned int>(cursor);
}
unsigned int __stdcall FSOUND_GetLoopMode(int id) {
  Channel* channel = Find(id);
  return channel ? channel->loopMode : FSOUND_LOOP_OFF;
}
signed char __stdcall FSOUND_SetLoopMode(int id, unsigned int mode) {
  Channel* channel = Find(id);
  if (!channel) return 0;
  channel->loopMode = mode;
  ApplyLoop(channel);
  return 1;
}
int __stdcall FSOUND_GetPriority(int id) {
  Channel* channel = Find(id);
  return channel ? channel->sample->priority : 0;
}
signed char __stdcall FSOUND_3D_SetAttributes(int id, const float* pos,
    const float*) {
  Channel* channel = Find(id);
  if (!channel || !pos) return 0;
  ma_sound_set_position(&channel->sound, pos[0], pos[1], pos[2]);
  return 1;
}
void __stdcall FSOUND_3D_Listener_SetAttributes(const float*, const float*,
    float, float, float, float, float, float) {}

// Native CStream bypasses this legacy FMOD streaming API. Keep link-complete
// definitions for the compiled but unreachable legacy callback path.
FSOUND_STREAM* __stdcall FSOUND_Stream_Open(const char*, unsigned int, int, int) { return nullptr; }
signed char __stdcall FSOUND_Stream_Close(FSOUND_STREAM*) { return 0; }
int __stdcall FSOUND_Stream_Play(int, FSOUND_STREAM*) { return -1; }
signed char __stdcall FSOUND_Stream_Stop(FSOUND_STREAM*) { return 0; }
signed char __stdcall FSOUND_Stream_SetTime(FSOUND_STREAM*, int) { return 0; }
int __stdcall FSOUND_Stream_GetTime(FSOUND_STREAM*) { return 0; }
int __stdcall FSOUND_Stream_GetLengthMs(FSOUND_STREAM*) { return 0; }
signed char __stdcall FSOUND_Stream_SetPosition(FSOUND_STREAM*, unsigned int) { return 0; }
signed char __stdcall FSOUND_Stream_SetSyncCallback(FSOUND_STREAM*,
    FSOUND_STREAMCALLBACK, intptr_t) { return 0; }
}
