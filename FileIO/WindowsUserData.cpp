#include "WindowsUserData.h"
#include "PortableUserPaths.h"

#include <windows.h>
#include <vector>

namespace S2FileIO {
namespace {
std::wstring EnvironmentValue(const wchar_t* name) {
  const DWORD length = GetEnvironmentVariableW(name, nullptr, 0);
  if (!length) return {};
  std::vector<wchar_t> value(length);
  if (GetEnvironmentVariableW(name, value.data(), length) >= length) return {};
  return value.data();
}

bool EnsureDirectory(const std::wstring& path) {
  if (path.size() < 3 || path[1] != L':') return false;
  for (std::size_t pos = 3; pos <= path.size(); ++pos) {
    if (pos != path.size() && path[pos] != L'\\') continue;
    const std::wstring part = path.substr(0, pos);
    if (!CreateDirectoryW(part.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
      return false;
    const DWORD attributes = GetFileAttributesW(part.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY) ||
        (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
  }
  return true;
}
} // namespace

const std::wstring& WindowsUserDataRoot() {
  static const std::wstring root = [] {
    WideUserPathEnvironment env;
    env.overrideRoot = EnvironmentValue(L"S2_USER_DATA_DIR");
    env.localAppData = EnvironmentValue(L"LOCALAPPDATA");
    std::wstring path;
    if (!ResolveUserDataRoot(HostPlatform::Windows, env, &path) || !EnsureDirectory(path))
      return std::wstring();
    return path;
  }();
  return root;
}

std::wstring WindowsConfigPath() {
  const std::wstring& root = WindowsUserDataRoot();
  if (root.empty()) return {};
  const std::wstring directory = root + L"\\cfg";
  if (!EnsureDirectory(directory)) return {};
  const std::wstring path = directory + L"\\config.cfg";
  if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
    CopyFileW(L".\\cfg\\config.cfg", path.c_str(), TRUE);
  const DWORD attributes = GetFileAttributesW(path.c_str());
  if (attributes != INVALID_FILE_ATTRIBUTES &&
      (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))) return {};
  return path;
}

std::wstring WindowsScreenshotDirectory() {
  const std::wstring& root = WindowsUserDataRoot();
  if (root.empty()) return {};
  const std::wstring directory = root + L"\\screenshots";
  return EnsureDirectory(directory) ? directory : std::wstring();
}
} // namespace S2FileIO
