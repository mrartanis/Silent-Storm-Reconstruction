#pragma once

#include <string>

namespace S2FileIO {

// Legacy slot identifiers are ACP bytes. Names that cannot round-trip through
// ACP are tagged as UTF-8; 0x1f cannot begin a valid Windows directory name.
std::string EncodeWindowsSaveName(const std::wstring& name);
bool DecodeWindowsSaveName(const std::string& encoded, std::wstring* name);

} // namespace S2FileIO
