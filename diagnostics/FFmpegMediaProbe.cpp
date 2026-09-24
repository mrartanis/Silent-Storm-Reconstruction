#include "../Media/FFmpegDecoder.h"
#include <cstdio>
#include <string>

int main(int argc, char** argv)
{
  if (argc != 3) {
    std::fprintf(stderr, "usage: FFmpegMediaProbe video|audio <file>\n");
    return 2;
  }
  const std::string kind(argv[1]);
  if (kind != "video" && kind != "audio") return 2;
  S2Media::FFmpegDecoder decoder;
  std::string error;
  if (!decoder.Open(argv[2], kind == "video" ? S2Media::StreamType::Video
                                            : S2Media::StreamType::Audio, &error)) {
    std::fprintf(stderr, "open failed: %s\n", error.c_str());
    return 1;
  }
  if (kind == "video") {
    S2Media::VideoFrame frame;
    if (!decoder.NextVideo(&frame) || !decoder.NextVideo(&frame) || frame.pixels.empty())
      return 1;
    std::printf("video %dx%d second-frame-ms=%lld bytes=%zu\n", frame.width,
                frame.height, static_cast<long long>(frame.timeMs), frame.pixels.size());
  } else {
    S2Media::AudioFrame frame;
    std::size_t samples = 0;
    while (samples < 22050 * 2 && decoder.NextAudio(&frame))
      samples += frame.samples.size();
    if (samples < 22050 * 2) return 1;
    std::printf("audio %d Hz channels=%d decoded-samples=%zu\n",
                frame.sampleRate, frame.channels, samples);
  }
  return 0;
}
