#include "FFmpegDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <cerrno>
#include <utility>

namespace S2Media {

namespace {
std::string ErrorText(int code)
{
  char text[AV_ERROR_MAX_STRING_SIZE] = {};
  av_strerror(code, text, sizeof(text));
  return text;
}
}

struct FFmpegDecoder::Impl {
  AVFormatContext* format = nullptr;
  AVCodecContext* codec = nullptr;
  AVPacket* packet = nullptr;
  AVFrame* frame = nullptr;
  SwsContext* scaler = nullptr;
  SwrContext* resampler = nullptr;
  int streamIndex = -1;
  StreamType type = StreamType::Video;
  bool draining = false;

  ~Impl()
  {
    swr_free(&resampler);
    sws_freeContext(scaler);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec);
    avformat_close_input(&format);
  }

  bool ReadFrame()
  {
    for (;;) {
      const int received = avcodec_receive_frame(codec, frame);
      if (received == 0) return true;
      if (received == AVERROR_EOF || received != AVERROR(EAGAIN)) return false;
      if (draining) return false;

      int read = 0;
      do {
        read = av_read_frame(format, packet);
        if (read < 0) {
          draining = true;
          avcodec_send_packet(codec, nullptr);
          break;
        }
        if (packet->stream_index == streamIndex) {
          const int sent = avcodec_send_packet(codec, packet);
          av_packet_unref(packet);
          if (sent < 0) return false;
          break;
        }
        av_packet_unref(packet);
      } while (true);
    }
  }

  std::int64_t TimeMs() const
  {
    if (frame->best_effort_timestamp == AV_NOPTS_VALUE) return 0;
    return av_rescale_q(frame->best_effort_timestamp,
                        format->streams[streamIndex]->time_base,
                        AVRational{1, 1000});
  }
};

FFmpegDecoder::FFmpegDecoder() = default;
FFmpegDecoder::~FFmpegDecoder() = default;

bool FFmpegDecoder::Open(const std::string& path, StreamType type,
                         std::string* error)
{
  Close();
  std::unique_ptr<Impl> candidate(new Impl);
  int result = avformat_open_input(&candidate->format, path.c_str(), nullptr, nullptr);
  if (result >= 0) result = avformat_find_stream_info(candidate->format, nullptr);
  if (result < 0) {
    if (error) *error = ErrorText(result);
    return false;
  }
  const AVMediaType mediaType = type == StreamType::Video ? AVMEDIA_TYPE_VIDEO : AVMEDIA_TYPE_AUDIO;
  const AVCodec* decoder = nullptr;
  candidate->streamIndex = av_find_best_stream(candidate->format, mediaType,
                                               -1, -1, &decoder, 0);
  if (candidate->streamIndex < 0 || !decoder) {
    if (error) *error = "requested media stream not found";
    return false;
  }
  candidate->codec = avcodec_alloc_context3(decoder);
  candidate->packet = av_packet_alloc();
  candidate->frame = av_frame_alloc();
  if (!candidate->codec || !candidate->packet || !candidate->frame) {
    if (error) *error = "FFmpeg allocation failed";
    return false;
  }
  result = avcodec_parameters_to_context(candidate->codec,
           candidate->format->streams[candidate->streamIndex]->codecpar);
  if (result >= 0) result = avcodec_open2(candidate->codec, decoder, nullptr);
  if (result < 0) {
    if (error) *error = ErrorText(result);
    return false;
  }
  candidate->type = type;
  impl = std::move(candidate);
  return true;
}

void FFmpegDecoder::Close() { impl.reset(); }

std::int64_t FFmpegDecoder::DurationMs() const
{
  return impl && impl->format->duration != AV_NOPTS_VALUE
           ? impl->format->duration / (AV_TIME_BASE / 1000) : 0;
}

bool FFmpegDecoder::SeekMs(std::int64_t timeMs)
{
  if (!impl || timeMs < 0) return false;
  const AVRational milliseconds{1, 1000};
  const std::int64_t target = av_rescale_q(timeMs, milliseconds,
      impl->format->streams[impl->streamIndex]->time_base);
  if (av_seek_frame(impl->format, impl->streamIndex, target,
                    AVSEEK_FLAG_BACKWARD) < 0)
    return false;
  avcodec_flush_buffers(impl->codec);
  impl->draining = false;
  av_frame_unref(impl->frame);
  return true;
}

int FFmpegDecoder::VideoWidth() const
{
  return impl && impl->type == StreamType::Video ? impl->codec->width : 0;
}
int FFmpegDecoder::VideoHeight() const
{
  return impl && impl->type == StreamType::Video ? impl->codec->height : 0;
}
int FFmpegDecoder::VideoFrameRateNumerator() const
{
  return impl && impl->type == StreamType::Video ?
      impl->format->streams[impl->streamIndex]->avg_frame_rate.num : 0;
}
int FFmpegDecoder::VideoFrameRateDenominator() const
{
  return impl && impl->type == StreamType::Video ?
      impl->format->streams[impl->streamIndex]->avg_frame_rate.den : 0;
}
std::int64_t FFmpegDecoder::VideoFrameCount() const
{
  if (!impl || impl->type != StreamType::Video) return 0;
  const AVStream* stream = impl->format->streams[impl->streamIndex];
  if (stream->nb_frames > 0) return stream->nb_frames;
  const AVRational rate = stream->avg_frame_rate;
  return rate.num > 0 && rate.den > 0 ?
      DurationMs() * rate.num / (1000LL * rate.den) : 0;
}

bool FFmpegDecoder::NextVideo(VideoFrame* output)
{
  if (!output || !impl || impl->type != StreamType::Video || !impl->ReadFrame())
    return false;
  const AVFrame* source = impl->frame;
  if (source->width <= 0 || source->height <= 0) return false;
  impl->scaler = sws_getCachedContext(impl->scaler, source->width, source->height,
      static_cast<AVPixelFormat>(source->format), source->width, source->height,
      AV_PIX_FMT_BGRA, SWS_BILINEAR, nullptr, nullptr, nullptr);
  if (!impl->scaler) return false;
  output->width = source->width;
  output->height = source->height;
  output->timeMs = impl->TimeMs();
  output->pixels.resize(static_cast<std::size_t>(source->width) * source->height * 4);
  std::uint8_t* planes[4] = { output->pixels.data(), nullptr, nullptr, nullptr };
  int stride[4] = { source->width * 4, 0, 0, 0 };
  return sws_scale(impl->scaler, source->data, source->linesize, 0,
                   source->height, planes, stride) == source->height;
}

bool FFmpegDecoder::NextAudio(AudioFrame* output)
{
  if (!output || !impl || impl->type != StreamType::Audio || !impl->ReadFrame())
    return false;
  const AVFrame* source = impl->frame;
  if (source->nb_samples <= 0 || source->sample_rate <= 0) return false;
  if (!impl->resampler) {
    AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
    AVChannelLayout input = source->ch_layout;
    if (!input.nb_channels) av_channel_layout_default(&input, 2);
    const int result = swr_alloc_set_opts2(&impl->resampler, &stereo,
        AV_SAMPLE_FMT_FLT, source->sample_rate, &input,
        static_cast<AVSampleFormat>(source->format), source->sample_rate, 0, nullptr);
    if (result < 0 || !impl->resampler || swr_init(impl->resampler) < 0)
      return false;
  }
  const int capacity = swr_get_out_samples(impl->resampler, source->nb_samples);
  if (capacity < 0) return false;
  output->samples.resize(static_cast<std::size_t>(capacity) * 2);
  std::uint8_t* data = reinterpret_cast<std::uint8_t*>(output->samples.data());
  const int count = swr_convert(impl->resampler, &data, capacity,
      const_cast<const std::uint8_t**>(source->extended_data), source->nb_samples);
  if (count < 0) return false;
  output->samples.resize(static_cast<std::size_t>(count) * 2);
  output->sampleRate = source->sample_rate;
  output->channels = 2;
  output->timeMs = impl->TimeMs();
  return true;
}

} // namespace S2Media
