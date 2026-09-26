#pragma once

#include <string>

namespace S2FileIO {

// Host adapter for the portable Linux user-path policy. Empty on failure.
const std::string& LinuxUserDataRoot();
std::string LinuxConfigPath();

// ASCII config representation of a profile name. New values use S2U8 hex;
// untagged legacy ASCII values remain readable.
std::string EncodeLinuxProfileConfig(const std::wstring& name);
bool DecodeLinuxProfileConfig(const std::string& encoded, std::wstring* name);

} // namespace S2FileIO
