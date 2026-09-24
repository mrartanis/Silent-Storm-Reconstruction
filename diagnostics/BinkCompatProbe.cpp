#include <bink.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

int main(int argc, char** argv)
{
  if (argc != 2) {
    std::fprintf(stderr, "usage: BinkCompatProbe <original .bik file>\n");
    return 2;
  }
  HBINK movie = BinkOpen(argv[1], 0);
  if (!movie || !movie->Width || !movie->Height || !movie->Frames ||
      !BinkDoFrame(movie)) return 1;
  const int pitch = static_cast<int>(movie->Width * 4);
  std::vector<std::uint8_t> pixels(static_cast<std::size_t>(pitch) * movie->Height);
  if (!BinkCopyToBuffer(movie, pixels.data(), pitch, movie->Height, 0, 0, 5))
    return 1;
  if (std::all_of(pixels.begin(), pixels.end(), [](std::uint8_t value) {
        return value == 0;
      })) return 1;
  const std::vector<std::uint8_t> first = pixels;
  const int width = movie->Width, height = movie->Height, frames = movie->Frames;
  BinkNextFrame(movie);
  if (!BinkDoFrame(movie)) return 1;
  std::vector<std::uint8_t> second(pixels.size());
  if (!BinkCopyToBuffer(movie, second.data(), pitch, movie->Height, 0, 0, 5))
    return 1;
  BinkPause(movie, 1);
  if (BinkWait(movie) != 1) return 1;
  BinkPause(movie, 0);
  BinkGoto(movie, 2, 0);
  if (!BinkDoFrame(movie)) return 1;
  if (!BinkCopyToBuffer(movie, pixels.data(), pitch, movie->Height, 0, 0, 5) ||
      pixels != second) return 1;
  BinkGoto(movie, frames, 0);
  if (movie->FrameNum != static_cast<U32>(frames) || !BinkDoFrame(movie) ||
      !BinkCopyToBuffer(movie, pixels.data(), pitch, movie->Height, 0, 0, 5))
    return 1;
  BinkNextFrame(movie);
  if (movie->FrameNum != 1 || !BinkDoFrame(movie) ||
      !BinkCopyToBuffer(movie, pixels.data(), pitch, movie->Height, 0, 0, 5) ||
      pixels != first) return 1;
  BinkClose(movie);
  std::printf("Bink bridge decoded %dx%d, %d frames; seek, pause and loop OK\n",
              width, height, frames);
  return 0;
}
