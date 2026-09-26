#include "LinuxUserData.h"
#include "PortableUserPaths.h"

#include <codecvt>
#include <cstdlib>
#include <filesystem>
#include <locale>
#include <system_error>

namespace S2FileIO {
namespace {
namespace fs = std::filesystem;

std::string EnvironmentValue(const char* name) {
  const char* value = std::getenv(name);
  return value ? value : "";
}

bool EnsureDirectory(const fs::path& path) {
  if (!path.is_absolute()) return false;
  fs::path current = path.root_path();
  std::error_code error;
  for (const auto& component : path.relative_path()) {
    current /= component;
    fs::file_status status = fs::symlink_status(current, error);
    if (error == std::errc::no_such_file_or_directory) error.clear();
    if (error) return false;
    if (status.type() == fs::file_type::not_found) {
      if (!fs::create_directory(current, error) || error) return false;
      status = fs::symlink_status(current, error);
      if (error) return false;
    }
    if (fs::is_symlink(status) || !fs::is_directory(status)) return false;
  }
  return true;
}

std::string ToUtf8(const std::wstring& value) {
  try {
    return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(value);
  } catch (const std::range_error&) {
    return {};
  }
}

bool FromUtf8(const std::string& value, std::wstring* result) {
  try {
    *result = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(value);
    return true;
  } catch (const std::range_error&) {
    return false;
  }
}
} // namespace

const std::string& LinuxUserDataRoot() {
  static const std::string root = [] {
    UserPathEnvironment env;
    env.overrideRoot = EnvironmentValue("S2_USER_DATA_DIR");
    env.home = EnvironmentValue("HOME");
    env.xdgDataHome = EnvironmentValue("XDG_DATA_HOME");
    std::string path;
    if (!ResolveUserDataRoot(HostPlatform::Linux, env, &path) ||
        !EnsureDirectory(fs::path(path))) return std::string();
    return path;
  }();
  return root;
}

std::string LinuxConfigPath() {
  const std::string& root = LinuxUserDataRoot();
  if (root.empty()) return {};
  const fs::path directory = fs::path(root) / "cfg";
  if (!EnsureDirectory(directory)) return {};
  const fs::path path = directory / "config.cfg";
  std::error_code error;
  fs::file_status status = fs::symlink_status(path, error);
  if (error == std::errc::no_such_file_or_directory) error.clear();
  if (error) return {};
  if (status.type() == fs::file_type::not_found) {
    const fs::path templatePath = fs::path("cfg") / "config.cfg";
    const fs::file_status source = fs::symlink_status(templatePath, error);
    if (error == std::errc::no_such_file_or_directory) error.clear();
    if (!error && fs::is_regular_file(source) && !fs::is_symlink(source))
      fs::copy_file(templatePath, path, fs::copy_options::skip_existing, error);
    if (error) return {};
    status = fs::symlink_status(path, error);
    if (error == std::errc::no_such_file_or_directory) error.clear();
    if (error) return {};
  }
  if (status.type() != fs::file_type::not_found &&
      (fs::is_symlink(status) || !fs::is_regular_file(status))) {
    return {};
  }
  return path.string();
}

std::string EncodeLinuxProfileConfig(const std::wstring& name) {
  const std::string utf8 = ToUtf8(name);
  if (!name.empty() && utf8.empty()) return {};
  static const char hex[] = "0123456789ABCDEF";
  std::string encoded = "S2U8:";
  encoded.reserve(encoded.size() + utf8.size() * 2);
  for (unsigned char byte : utf8) {
    encoded += hex[byte >> 4];
    encoded += hex[byte & 15];
  }
  return encoded;
}

bool DecodeLinuxProfileConfig(const std::string& encoded, std::wstring* name) {
  if (!name) return false;
  if (encoded.compare(0, 5, "S2U8:") != 0) {
    // Historical untagged profiles were ASCII on Linux. Reject ambiguous
    // high bytes rather than silently interpreting a Windows ACP as UTF-8.
    for (unsigned char byte : encoded) if (byte > 0x7f) return false;
    name->assign(encoded.begin(), encoded.end());
    return true;
  }
  if ((encoded.size() - 5) % 2) return false;
  const auto digit = [](char ch) -> int {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return -1;
  };
  std::string utf8;
  utf8.reserve((encoded.size() - 5) / 2);
  for (std::size_t offset = 5; offset < encoded.size(); offset += 2) {
    const int high = digit(encoded[offset]), low = digit(encoded[offset + 1]);
    if (high < 0 || low < 0) return false;
    utf8 += static_cast<char>((high << 4) | low);
  }
  return FromUtf8(utf8, name);
}

} // namespace S2FileIO
