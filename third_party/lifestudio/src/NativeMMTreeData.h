#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace NativeLifeStudio
{
struct MMTreeOperationRecord
{
  std::size_t offset = 0;
  std::uint32_t headerWords[8] = {};
  std::uint32_t payloadSize = 0;
  std::uint32_t preludeSize = 0;
  std::uint32_t serializedType = 0;
  std::string name;
  std::vector<MMTreeOperationRecord> children;
};

struct MMTreeRoot
{
  std::string name;
  std::uint32_t preludeSize = 0;
  std::vector<MMTreeOperationRecord> operations;
};

// Decode the MMLF v4 envelope and nested length-delimited operation graph.
// This does not interpret effect payloads or apply any facial deformation.
bool DecodeMMTreeRoot(const void *bytes, std::size_t size, MMTreeRoot *result);
}
