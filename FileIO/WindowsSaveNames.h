#pragma once

#include <string>

namespace S2FileIO {

// Legacy profile/slot identifiers are ACP bytes. Names that cannot round-trip
// through ACP are tagged as UTF-8; 0x1f cannot begin a Windows directory name.
std::string EncodeWindowsSaveName(const std::wstring& name);
bool DecodeWindowsSaveName(const std::string& encoded, std::wstring* name);
// ASCII-only config representation for game_profile; legacy untagged values
// remain readable. ':' cannot occur in an actual Windows profile directory.
std::string EncodeWindowsProfileConfig(const std::wstring& name);
bool DecodeWindowsProfileConfig(const std::string& encoded, std::wstring* name);

} // namespace S2FileIO
