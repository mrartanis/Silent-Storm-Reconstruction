#include "PortableUserPaths.h"

#include <cctype>
#include <cwctype>

namespace S2FileIO {
namespace {
bool Fail(std::string* error, const char* why) {
  if (error) *error = why;
  return false;
}

bool IsAbsolute(HostPlatform platform, const std::string& path) {
  if (platform == HostPlatform::Windows)
    return path.size() >= 3 && std::isalpha(static_cast<unsigned char>(path[0])) &&
           path[1] == ':' && (path[2] == '\\' || path[2] == '/');
  return !path.empty() && path[0] == '/';
}

std::string Normalize(HostPlatform platform, std::string path) {
  const char separator = platform == HostPlatform::Windows ? '\\' : '/';
  const char other = separator == '\\' ? '/' : '\\';
  for (char& ch : path)
    if (ch == other) ch = separator;
  while (path.size() > (platform == HostPlatform::Windows ? 3u : 1u) &&
         path.back() == separator)
    path.pop_back();
  return path;
}

std::string Append(HostPlatform platform, const std::string& base,
                   const char* child) {
  std::string path = Normalize(platform, base);
  if (path.empty()) return child;
  const char separator = platform == HostPlatform::Windows ? '\\' : '/';
  if (path.back() != separator) path += separator;
  return path + child;
}

bool HasDotComponent(const std::string& path, char separator) {
  std::size_t begin = 0;
  while (begin < path.size()) {
    const std::size_t end = path.find(separator, begin);
    const std::string part = path.substr(begin, end == std::string::npos ? end : end - begin);
    if (part == "." || part == "..") return true;
    if (end == std::string::npos) break;
    begin = end + 1;
  }
  return false;
}

bool IsAbsolute(HostPlatform platform, const std::wstring& path) {
  if (platform == HostPlatform::Windows)
    return path.size() >= 3 && std::iswalpha(path[0]) &&
           path[1] == L':' && (path[2] == L'\\' || path[2] == L'/');
  return !path.empty() && path[0] == L'/';
}

std::wstring Normalize(HostPlatform platform, std::wstring path) {
  const wchar_t separator = platform == HostPlatform::Windows ? L'\\' : L'/';
  const wchar_t other = separator == L'\\' ? L'/' : L'\\';
  for (wchar_t& ch : path)
    if (ch == other) ch = separator;
  while (path.size() > (platform == HostPlatform::Windows ? 3u : 1u) &&
         path.back() == separator)
    path.pop_back();
  return path;
}

std::wstring Append(HostPlatform platform, const std::wstring& base,
                    const wchar_t* child) {
  std::wstring path = Normalize(platform, base);
  if (path.empty()) return child;
  const wchar_t separator = platform == HostPlatform::Windows ? L'\\' : L'/';
  if (path.back() != separator) path += separator;
  return path + child;
}

bool HasDotComponent(const std::wstring& path, wchar_t separator) {
  std::size_t begin = 0;
  while (begin < path.size()) {
    const std::size_t end = path.find(separator, begin);
    const std::wstring part = path.substr(begin, end == std::wstring::npos ? end : end - begin);
    if (part == L"." || part == L"..") return true;
    if (end == std::wstring::npos) break;
    begin = end + 1;
  }
  return false;
}
} // namespace

bool ResolveUserDataRoot(HostPlatform platform, const UserPathEnvironment& env,
                         std::string* result, std::string* error) {
  if (!result) return Fail(error, "missing result");
  result->clear();
  const char separator = platform == HostPlatform::Windows ? '\\' : '/';
  std::string root;
  if (!env.overrideRoot.empty()) {
    root = env.overrideRoot;
  } else if (platform == HostPlatform::Windows) {
    if (env.localAppData.empty()) return Fail(error, "LOCALAPPDATA is unset");
    root = Append(platform, env.localAppData, "Silent Storm Reconstruction");
  } else if (platform == HostPlatform::MacOS) {
    if (env.home.empty()) return Fail(error, "HOME is unset");
    root = Append(platform, env.home, "Library/Application Support/Silent Storm Reconstruction");
  } else {
    if (!env.xdgDataHome.empty()) root = Append(platform, env.xdgDataHome, "silent-storm-reconstruction");
    else {
      if (env.home.empty()) return Fail(error, "HOME is unset");
      root = Append(platform, env.home, ".local/share/silent-storm-reconstruction");
    }
  }
  root = Normalize(platform, root);
  if (!IsAbsolute(platform, root) || HasDotComponent(root, separator))
    return Fail(error, "user data root must be absolute and contain no dot components");
  *result = root;
  return true;
}

bool IsSafeSaveComponent(const std::string& component) {
  if (component.empty() || component == "." || component == "..") return false;
  if (component.back() == '.' || component.back() == ' ') return false;
  for (unsigned char ch : component)
    if (ch < 32 || ch == '/' || ch == '\\' || ch == ':' || ch == '<' || ch == '>' ||
        ch == '|' || ch == '"' || ch == '*' || ch == '?')
      return false;
  // Win32 treats these device names as special even with a file extension.
  std::string stem = component.substr(0, component.find('.'));
  for (char& ch : stem)
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL") return false;
  if (stem.size() == 4 && (stem.substr(0, 3) == "COM" || stem.substr(0, 3) == "LPT") &&
      stem[3] >= '1' && stem[3] <= '9') return false;
  return true;
}

bool ResolveUserDataRoot(HostPlatform platform, const WideUserPathEnvironment& env,
                         std::wstring* result, std::string* error) {
  if (!result) return Fail(error, "missing result");
  result->clear();
  const wchar_t separator = platform == HostPlatform::Windows ? L'\\' : L'/';
  std::wstring root;
  if (!env.overrideRoot.empty()) root = env.overrideRoot;
  else if (platform == HostPlatform::Windows) {
    if (env.localAppData.empty()) return Fail(error, "LOCALAPPDATA is unset");
    root = Append(platform, env.localAppData, L"Silent Storm Reconstruction");
  } else if (platform == HostPlatform::MacOS) {
    if (env.home.empty()) return Fail(error, "HOME is unset");
    root = Append(platform, env.home, L"Library/Application Support/Silent Storm Reconstruction");
  } else {
    if (!env.xdgDataHome.empty()) root = Append(platform, env.xdgDataHome, L"silent-storm-reconstruction");
    else {
      if (env.home.empty()) return Fail(error, "HOME is unset");
      root = Append(platform, env.home, L".local/share/silent-storm-reconstruction");
    }
  }
  root = Normalize(platform, root);
  if (!IsAbsolute(platform, root) || HasDotComponent(root, separator))
    return Fail(error, "user data root must be absolute and contain no dot components");
  *result = root;
  return true;
}

bool IsSafeSaveComponent(const std::wstring& component) {
  if (component.empty() || component == L"." || component == L"..") return false;
  if (component.back() == L'.' || component.back() == L' ') return false;
  for (wchar_t ch : component)
    if (ch < 32 || ch == L'/' || ch == L'\\' || ch == L':' || ch == L'<' || ch == L'>' ||
        ch == L'|' || ch == L'"' || ch == L'*' || ch == L'?')
      return false;
  std::wstring stem = component.substr(0, component.find(L'.'));
  for (wchar_t& ch : stem) ch = std::towupper(ch);
  if (stem == L"CON" || stem == L"PRN" || stem == L"AUX" || stem == L"NUL") return false;
  if (stem.size() == 4 && (stem.substr(0, 3) == L"COM" || stem.substr(0, 3) == L"LPT") &&
      stem[3] >= L'1' && stem[3] <= L'9') return false;
  return true;
}

} // namespace S2FileIO
