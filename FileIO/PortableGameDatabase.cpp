#include "PortableGameDatabase.h"
#include "PortableStructureChunks.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>

namespace S2FileIO {
namespace {
struct View {
  const std::uint8_t* bytes = nullptr;
  std::size_t size = 0;
};

bool Fail(std::string* error, const char* message) {
  if (error) *error = message;
  return false;
}

View Slice(View parent, const StructureChunk& chunk) {
  return {parent.bytes + chunk.payloadOffset, chunk.length};
}

bool Children(View view, std::vector<StructureChunk>* chunks) {
  chunks->clear();
  std::size_t offset = 0;
  while (offset < view.size) {
    StructureChunk chunk;
    if (!DecodeStructureChunkAt(view.bytes, view.size, offset, &chunk)) return false;
    chunks->push_back(chunk);
    offset = static_cast<std::size_t>(chunk.payloadOffset) + chunk.length;
  }
  return true;
}

bool One(View view, std::uint8_t id, View* result) {
  std::vector<StructureChunk> chunks;
  if (!Children(view, &chunks)) return false;
  bool found = false;
  for (const auto& chunk : chunks) {
    if (chunk.id != id) continue;
    if (found) return false;
    *result = Slice(view, chunk);
    found = true;
  }
  return found;
}

bool ReadU32(View view, std::uint32_t* value) {
  if (view.size != 4) return false;
  *value = std::uint32_t(view.bytes[0]) | (std::uint32_t(view.bytes[1]) << 8) |
           (std::uint32_t(view.bytes[2]) << 16) | (std::uint32_t(view.bytes[3]) << 24);
  return true;
}

std::int32_t Signed(std::uint32_t value) {
  return static_cast<std::int32_t>(value <= INT32_MAX ? std::int64_t(value) :
                                    std::int64_t(value) - INT64_C(0x100000000));
}

bool ReadI32(View view, std::int32_t* value) {
  std::uint32_t raw = 0;
  if (!ReadU32(view, &raw)) return false;
  *value = Signed(raw);
  return true;
}

std::string ReadString(View view) {
  return view.size ? std::string(reinterpret_cast<const char*>(view.bytes), view.size)
                   : std::string();
}

bool PackedRow(View view, std::vector<std::uint32_t>* values) {
  View countBytes, dataBytes;
  std::int32_t count = 0;
  if (!One(view, 1, &countBytes) || !ReadI32(countBytes, &count) || count < 0)
    return false;
  if (!count) {
    values->clear();
    std::vector<StructureChunk> chunks;
    return Children(view, &chunks) && chunks.size() == 1;
  }
  if (!One(view, 2, &dataBytes) ||
      static_cast<std::uint64_t>(count) * 4 != dataBytes.size) return false;
  values->resize(static_cast<std::size_t>(count));
  for (std::size_t i = 0; i < values->size(); ++i) {
    View element{dataBytes.bytes + i * 4, 4};
    if (!ReadU32(element, &(*values)[i])) return false;
  }
  return true;
}

bool IntRows(View view, std::vector<std::vector<std::int32_t>>* rows) {
  std::vector<StructureChunk> chunks;
  if (!Children(view, &chunks)) return false;
  rows->clear();
  rows->reserve(chunks.size());
  for (const auto& chunk : chunks) {
    if (chunk.id != 1) return false;
    std::vector<std::uint32_t> bits;
    if (!PackedRow(Slice(view, chunk), &bits)) return false;
    rows->emplace_back();
    auto& row = rows->back();
    row.reserve(bits.size());
    for (std::uint32_t value : bits) row.push_back(Signed(value));
  }
  return true;
}

bool FloatRows(View view, std::vector<std::vector<float>>* rows) {
  static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
                "game.db requires IEEE-754 binary32");
  std::vector<StructureChunk> chunks;
  if (!Children(view, &chunks)) return false;
  rows->clear();
  rows->reserve(chunks.size());
  for (const auto& chunk : chunks) {
    if (chunk.id != 1) return false;
    std::vector<std::uint32_t> bits;
    if (!PackedRow(Slice(view, chunk), &bits)) return false;
    rows->emplace_back();
    auto& row = rows->back();
    row.reserve(bits.size());
    for (std::uint32_t value : bits) {
      float decoded = 0;
      std::memcpy(&decoded, &value, sizeof(decoded));
      row.push_back(decoded);
    }
  }
  return true;
}

bool StringRows(View view, std::vector<std::vector<std::wstring>>* rows) {
  std::vector<StructureChunk> chunks;
  if (!Children(view, &chunks)) return false;
  rows->clear();
  rows->reserve(chunks.size());
  for (const auto& chunk : chunks) {
    if (chunk.id != 1) return false;
    const View rowView = Slice(view, chunk);
    std::vector<StructureChunk> cells;
    if (!Children(rowView, &cells)) return false;
    rows->emplace_back();
    auto& row = rows->back();
    row.reserve(cells.size());
    for (const auto& cell : cells) {
      if (cell.id != 1) return false;
      const View content = Slice(rowView, cell);
      row.emplace_back();
      if (!DecodeStructureUtf16(content.bytes, content.size, &row.back())) return false;
    }
  }
  return true;
}

bool Names(View view, std::vector<std::string>* names) {
  std::vector<StructureChunk> chunks;
  if (!Children(view, &chunks)) return false;
  names->clear();
  names->reserve(chunks.size());
  for (const auto& chunk : chunks) {
    if (chunk.id != 1) return false;
    names->push_back(ReadString(Slice(view, chunk)));
  }
  return true;
}

bool Columns(View view, std::vector<GameDatabaseColumn>* columns) {
  std::vector<StructureChunk> chunks;
  if (!Children(view, &chunks)) return false;
  columns->clear();
  columns->reserve(chunks.size());
  for (const auto& chunk : chunks) {
    if (chunk.id != 1) return false;
    const View column = Slice(view, chunk);
    View name, type;
    if (!One(column, 2, &name) || !One(column, 3, &type)) return false;
    columns->push_back({ReadString(name), 0});
    if (!ReadI32(type, &columns->back().type)) return false;
  }
  return true;
}

bool Table(View view, GameDatabaseTable* table) {
  View ints, floats, strings, fields, intNames, floatNames, stringNames;
  return One(view, 2, &ints) && One(view, 3, &floats) &&
         One(view, 4, &strings) && One(view, 5, &fields) &&
         One(view, 6, &intNames) && One(view, 7, &floatNames) &&
         One(view, 8, &stringNames) &&
         IntRows(ints, &table->intRows) && FloatRows(floats, &table->floatRows) &&
         StringRows(strings, &table->stringRows) && Columns(fields, &table->columns) &&
         Names(intNames, &table->intNames) && Names(floatNames, &table->floatNames) &&
         Names(stringNames, &table->stringNames);
}

bool Hash(View view, std::map<std::uint32_t, std::uint32_t>* tableWires) {
  std::vector<StructureChunk> chunks;
  if (!Children(view, &chunks)) return false;
  std::vector<std::uint32_t> keys, values;
  for (const auto& chunk : chunks) {
    std::uint32_t value = 0;
    if (!ReadU32(Slice(view, chunk), &value)) return false;
    if (chunk.id == 1) keys.push_back(value);
    else if (chunk.id == 2) values.push_back(value);
    else return false;
  }
  if (keys.size() != values.size()) return false;
  tableWires->clear();
  for (std::size_t i = 0; i < keys.size(); ++i)
    if (!tableWires->emplace(keys[i], values[i]).second) return false;
  return true;
}

bool Relations(View view, std::vector<GameDatabaseRelation>* relations) {
  std::vector<StructureChunk> chunks;
  if (!Children(view, &chunks)) return false;
  relations->clear();
  relations->reserve(chunks.size());
  for (const auto& chunk : chunks) {
    if (chunk.id != 1) return false;
    const View relation = Slice(view, chunk);
    View name, left, right, links;
    if (!One(relation, 2, &name) || !One(relation, 3, &left) ||
        !One(relation, 4, &right) || !One(relation, 5, &links)) return false;
    relations->emplace_back();
    auto& result = relations->back();
    result.name = ReadString(name);
    if (!ReadI32(left, &result.leftTableId) ||
        !ReadI32(right, &result.rightTableId)) return false;
    View countBytes;
    std::int32_t count = 0;
    if (!One(links, 1, &countBytes) || !ReadI32(countBytes, &count) || count < 0)
      return false;
    if (!count) continue;
    View linkBytes;
    if (!One(links, 2, &linkBytes) ||
        static_cast<std::uint64_t>(count) * 8 != linkBytes.size) return false;
    result.links.reserve(static_cast<std::size_t>(count));
    for (std::size_t i = 0; i < static_cast<std::size_t>(count); ++i) {
      std::uint32_t a = 0, b = 0;
      if (!ReadU32({linkBytes.bytes + 8 * i, 4}, &a) ||
          !ReadU32({linkBytes.bytes + 8 * i + 4, 4}, &b)) return false;
      result.links.emplace_back(Signed(a), Signed(b));
    }
  }
  return true;
}
} // namespace

bool LoadPortableGameDatabase(const std::string& path, PortableGameDatabase* database,
                              std::string* error) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) return Fail(error, "cannot open game.db");
  const auto end = file.tellg();
  if (end <= 0 || static_cast<std::uint64_t>(end) > SIZE_MAX)
    return Fail(error, "invalid game.db size");
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
  file.seekg(0);
  if (!file.read(reinterpret_cast<char*>(bytes.data()), bytes.size()))
    return Fail(error, "cannot read game.db");
  return LoadPortableGameDatabaseBytes(bytes.data(), bytes.size(), database, error);
}

bool LoadPortableGameDatabaseBytes(const std::uint8_t* bytes, std::size_t length,
                                   PortableGameDatabase* database,
                                   std::string* error) {
  if (!database) return Fail(error, "missing database output");
  database->tables.clear();
  database->relations.clear();
  if (length && !bytes) return Fail(error, "missing game.db bytes");
  const View whole{bytes, length};
  View version, main, objectTable, objectData;
  if (!One(whole, 4, &version) || !One(whole, 1, &main) ||
      !One(whole, 0, &objectTable) || !One(whole, 2, &objectData))
    return Fail(error, "missing game.db root chunks");
  std::uint32_t formatVersion = 0;
  if (!ReadU32(version, &formatVersion) || formatVersion != 1)
    return Fail(error, "unsupported game.db version");
  std::vector<StructureObjectRecord> records;
  std::vector<StructureObjectBody> bodies;
  if (!DecodeStructureObjectTable(objectTable.bytes, objectTable.size, &records) ||
      !IndexStructureObjectBodies(objectData.bytes, objectData.size, &bodies))
    return Fail(error, "invalid game.db object graph");
  std::map<std::uint32_t, View> bodyByWire;
  for (const auto& body : bodies) {
    if (!bodyByWire.emplace(body.wireId,
          View{objectData.bytes + body.bodyOffset, body.bodyLength}).second)
      return Fail(error, "duplicate game.db object body");
  }
  if (records.size() != bodies.size())
    return Fail(error, "game.db object table/body count mismatch");
  for (const auto& record : records)
    if (!record.valid || record.typeId != UINT32_C(0xa1843130) ||
        !bodyByWire.count(record.wireId))
      return Fail(error, "unsupported game.db object type");
  View hashView, relationView;
  if (!One(main, 1, &hashView) || !One(main, 2, &relationView))
    return Fail(error, "missing game.db table map or relations");
  std::map<std::uint32_t, std::uint32_t> tableWires;
  if (!Hash(hashView, &tableWires) || !Relations(relationView, &database->relations))
    return Fail(error, "invalid game.db table map or relations");
  if (tableWires.size() != records.size())
    return Fail(error, "game.db table map/object count mismatch");
  for (const auto& item : tableWires) {
    const auto found = bodyByWire.find(item.second);
    if (found == bodyByWire.end()) return Fail(error, "missing game.db table body");
    database->tables.emplace_back();
    auto& table = database->tables.back();
    table.tableId = Signed(item.first);
    table.wireId = item.second;
    if (!Table(found->second, &table)) return Fail(error, "invalid game.db table columns");
  }
  return true;
}

bool HashGameDatabaseTable(const GameDatabaseTable& table, std::uint64_t* hash) {
  if (!hash) return false;
  std::uint64_t result = UINT64_C(14695981039346656037);
  const auto byte = [&result](std::uint8_t value) {
    result = (result ^ value) * UINT64_C(1099511628211);
  };
  const auto u32 = [&byte](std::uint32_t value) {
    for (unsigned i = 0; i != 4; ++i)
      byte(static_cast<std::uint8_t>(value >> (8 * i)));
  };
  const auto name = [&u32, &byte](const std::string& value) {
    u32(static_cast<std::uint32_t>(value.size()));
    for (unsigned char c : value) byte(c);
  };
  u32(static_cast<std::uint32_t>(table.tableId));
  for (const auto& column : table.columns) {
    name(column.name);
    u32(static_cast<std::uint32_t>(column.type));
  }
  for (const auto& item : table.intNames) name(item);
  for (const auto& item : table.floatNames) name(item);
  for (const auto& item : table.stringNames) name(item);
  for (const auto& row : table.intRows)
    for (std::int32_t value : row) u32(static_cast<std::uint32_t>(value));
  for (const auto& row : table.floatRows)
    for (float value : row) {
      std::uint32_t bits = 0;
      std::memcpy(&bits, &value, sizeof(bits));
      u32(bits);
    }
  for (const auto& row : table.stringRows)
    for (const auto& value : row) {
      std::vector<std::uint8_t> encoded;
      if (!EncodeStructureUtf16(value, &encoded)) return false;
      u32(static_cast<std::uint32_t>(encoded.size()));
      for (std::uint8_t item : encoded) byte(item);
    }
  *hash = result;
  return true;
}

bool HashGameDatabaseRelations(const std::vector<GameDatabaseRelation>& relations,
                               std::uint64_t* hash) {
  if (!hash) return false;
  std::uint64_t result = UINT64_C(14695981039346656037);
  const auto byte = [&result](std::uint8_t value) {
    result = (result ^ value) * UINT64_C(1099511628211);
  };
  const auto u32 = [&byte](std::uint32_t value) {
    for (unsigned i = 0; i != 4; ++i)
      byte(static_cast<std::uint8_t>(value >> (8 * i)));
  };
  u32(static_cast<std::uint32_t>(relations.size()));
  for (const auto& relation : relations) {
    u32(static_cast<std::uint32_t>(relation.name.size()));
    for (unsigned char c : relation.name) byte(c);
    u32(static_cast<std::uint32_t>(relation.leftTableId));
    u32(static_cast<std::uint32_t>(relation.rightTableId));
    u32(static_cast<std::uint32_t>(relation.links.size()));
    for (const auto& link : relation.links) {
      u32(static_cast<std::uint32_t>(link.first));
      u32(static_cast<std::uint32_t>(link.second));
    }
  }
  *hash = result;
  return true;
}

} // namespace S2FileIO
