#include "NativeSequenceData.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <vector>

static void U32(std::vector<unsigned char> *bytes, std::size_t offset, std::uint32_t value)
{
  for (int i = 0; i < 4; ++i)
    (*bytes)[offset + i] = static_cast<unsigned char>(value >> (i * 8));
}

int main()
{
  std::vector<unsigned char> good(40, 0);
  U32(&good, 0, 0x46534D4Du); // MMSF
  U32(&good, 4, 1);
  U32(&good, 8, 8);
  U32(&good, 12, 30000);
  U32(&good, 16, 2);
  U32(&good, 20, 100);
  NativeLifeStudio::SequenceHeader header;
  assert(NativeLifeStudio::DecodeSequenceHeader(good.data(), good.size(), &header));
  assert(header.payloadSize == 8 && header.duration == 30000 &&
         header.trackCount == 2 && header.preludeSize == 100);
  for (std::size_t n = 0; n < good.size(); ++n)
    assert(!NativeLifeStudio::DecodeSequenceHeader(good.data(), n, &header));
  auto bad = good;
  U32(&bad, 0, 0);
  assert(!NativeLifeStudio::DecodeSequenceHeader(bad.data(), bad.size(), &header));
  bad = good;
  U32(&bad, 4, 2);
  assert(!NativeLifeStudio::DecodeSequenceHeader(bad.data(), bad.size(), &header));
  bad = good;
  U32(&bad, 8, 0xFFFFFFFFu);
  assert(!NativeLifeStudio::DecodeSequenceHeader(bad.data(), bad.size(), &header));
  bad = good;
  U32(&bad, 12, 0);
  assert(!NativeLifeStudio::DecodeSequenceHeader(bad.data(), bad.size(), &header));
  bad = good;
  U32(&bad, 16, 0);
  assert(!NativeLifeStudio::DecodeSequenceHeader(bad.data(), bad.size(), &header));
  bad = good;
  U32(&bad, 16, 3);
  assert(!NativeLifeStudio::DecodeSequenceHeader(bad.data(), bad.size(), &header));
  assert(!NativeLifeStudio::DecodeSequenceHeader(nullptr, good.size(), &header));
  assert(!NativeLifeStudio::DecodeSequenceHeader(good.data(), good.size(), nullptr));

  std::vector<unsigned char> sequence(194, 0);
  U32(&sequence, 0, 0x46534D4Du);
  U32(&sequence, 4, 1);
  U32(&sequence, 8, 162);
  U32(&sequence, 12, 1500);
  U32(&sequence, 16, 2);
  U32(&sequence, 20, 76);
  U32(&sequence, 108 + 4, 17);
  U32(&sequence, 108 + 32, 5);
  for (int i = 0; i < 5; ++i) sequence[108 + 36 + i] = "Sound"[i];
  U32(&sequence, 157 + 4, 5);
  U32(&sequence, 157 + 32, 1);
  sequence[157 + 36] = 'A';
  std::vector<NativeLifeStudio::SequenceTrack> tracks;
  assert(NativeLifeStudio::DecodeSequenceTracks(sequence.data(), sequence.size(), &tracks));
  assert(tracks.size() == 2 && tracks[0].offset == 108 && tracks[0].payloadSize == 17 &&
         tracks[0].name == "Sound" && tracks[1].offset == 157 && tracks[1].name == "A");
  for (std::size_t n = 0; n < sequence.size(); ++n)
    assert(!NativeLifeStudio::DecodeSequenceTracks(sequence.data(), n, &tracks));
  bad = sequence;
  U32(&bad, 20, 77);
  assert(!NativeLifeStudio::DecodeSequenceTracks(bad.data(), bad.size(), &tracks));
  bad = sequence;
  U32(&bad, 108 + 4, 18);
  assert(!NativeLifeStudio::DecodeSequenceTracks(bad.data(), bad.size(), &tracks));
  bad = sequence;
  U32(&bad, 108 + 32, 18);
  assert(!NativeLifeStudio::DecodeSequenceTracks(bad.data(), bad.size(), &tracks));
  bad = sequence;
  bad[108 + 36] = 0;
  assert(!NativeLifeStudio::DecodeSequenceTracks(bad.data(), bad.size(), &tracks));
  bad = sequence;
  U32(&bad, 16, 1);
  assert(!NativeLifeStudio::DecodeSequenceTracks(bad.data(), bad.size(), &tracks));
  assert(!NativeLifeStudio::DecodeSequenceTracks(nullptr, sequence.size(), &tracks));
  assert(!NativeLifeStudio::DecodeSequenceTracks(sequence.data(), sequence.size(), nullptr));
  return 0;
}
