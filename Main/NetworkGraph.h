#pragma once
#include "NetworkTransport.h"
#include <map>

namespace S2Net {
using NetObjectId = std::uint32_t;
struct ObjectRecord {
  std::uint32_t type = 0;
  bool valid = true;
  Bytes body;
  bool operator==(const ObjectRecord& other) const {
    return type == other.type && valid == other.valid && body == other.body;
  }
};
// Records are sorted by permanent ID, independent of graph traversal order.
class NetworkGraph {
  std::map<NetObjectId,ObjectRecord> records;
  Bytes root;
  std::uint64_t revision = 0;
 public:
  // Host: compare serialized records, send only changed records and deletions.
  Bytes Capture(const Bytes& structure, bool initial);
  // Client: atomically validate and apply one ordered packet to the cache.
  void Apply(const Bytes& delta, bool initial);
  Bytes Structure() const;
  std::uint64_t Revision() const { return revision; }
  std::size_t Objects() const { return records.size(); }
};
}
