#include "../FileIO/PortableGroupWire.h"

#include <cstdint>
#include <cstring>

int main() {
  const S2FileIO::GroupInfoFields group{0x1234, 0x80ff};
  const std::uint8_t groupExpected[4] = {0x34, 0x12, 0xff, 0x80};
  std::uint8_t wire[4] = {};
  S2FileIO::GroupInfoFields decodedGroup{};
  if (!S2FileIO::EncodeGroupInfo(group, wire, sizeof(wire)) ||
      std::memcmp(wire, groupExpected, sizeof(wire)) != 0 ||
      !S2FileIO::DecodeGroupInfo(groupExpected, sizeof(groupExpected),
                                 &decodedGroup) ||
      decodedGroup.lightGroup != group.lightGroup ||
      decodedGroup.objectGroup != group.objectGroup ||
      S2FileIO::DecodeGroupInfo(groupExpected, 3, &decodedGroup) ||
      S2FileIO::EncodeGroupInfo(group, wire, 3)) return 1;

  const S2FileIO::GroupSelectFields select{0x8000, 0x0fff};
  const std::uint8_t selectExpected[4] = {0x00, 0x80, 0xff, 0x0f};
  S2FileIO::GroupSelectFields decodedSelect{};
  if (!S2FileIO::EncodeGroupSelect(select, wire, sizeof(wire)) ||
      std::memcmp(wire, selectExpected, sizeof(wire)) != 0 ||
      !S2FileIO::DecodeGroupSelect(selectExpected, sizeof(selectExpected),
                                   &decodedSelect) ||
      decodedSelect.maskAny != select.maskAny ||
      decodedSelect.maskEvery != select.maskEvery ||
      S2FileIO::DecodeGroupSelect(selectExpected, 3, &decodedSelect) ||
      S2FileIO::EncodeGroupSelect(select, wire, 3)) return 2;
  return 0;
}
