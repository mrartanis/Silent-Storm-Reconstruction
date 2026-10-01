#pragma once
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

namespace S2FileIO {
// Authored resource paths retain Windows separators and casing. Resolve them
// for reads on a case-sensitive host, while keeping native writable paths exact.
inline std::string ResolveGamePath(const char* name) {
  if (!name) return {};
  std::string normalized(name);
  std::replace(normalized.begin(), normalized.end(), '\\', '/');
  namespace fs = std::filesystem;
  const fs::path requested = fs::u8path(normalized);
  std::error_code error;
  if (fs::exists(requested, error)) return requested.u8string();
  fs::path result = requested.is_absolute() ? requested.root_path() : fs::path(".");
  const auto lower = [](std::string text) {
    for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
  };
  for (const auto& component : requested.relative_path()) {
    if (component == "." || component == "..") { result /= component; continue; }
    const fs::path exact = result / component;
    if (fs::exists(exact, error)) { result = exact; continue; }
    error.clear();
    fs::path match;
    for (const auto& entry : fs::directory_iterator(result, error)) {
      if (error) break;
      if (lower(entry.path().filename().u8string()) == lower(component.u8string())) {
        if (!match.empty()) return normalized; // An ambiguous spelling stays unresolved.
        match = entry.path();
      }
    }
    if (match.empty()) return normalized;
    result = match;
  }
  return result.u8string();
}
}
