#include "WindowsSaveNames.h"

#include <windows.h>

namespace S2FileIO {
namespace {
constexpr char kUtf8Tag = '\x1f';
// The historical project sets _WIN32_WINNT=0x0400, hiding this Vista+ flag.
constexpr DWORD kWideErrInvalidChars = 0x00000080;

bool ConvertToBytes(UINT codepage, DWORD flags, const std::wstring& input,
                    std::string* output) {
  if (input.empty()) { output->clear(); return true; }
  const int count = WideCharToMultiByte(codepage, flags, input.data(),
                                        static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
  if (count <= 0) return false;
  output->resize(count);
  return WideCharToMultiByte(codepage, flags, input.data(), static_cast<int>(input.size()),
                             &(*output)[0], count, nullptr, nullptr) == count;
}

bool ConvertToWide(UINT codepage, DWORD flags, const std::string& input,
                   std::wstring* output) {
  if (input.empty()) { output->clear(); return true; }
  const int count = MultiByteToWideChar(codepage, flags, input.data(),
                                        static_cast<int>(input.size()), nullptr, 0);
  if (count <= 0) return false;
  output->resize(count);
  return MultiByteToWideChar(codepage, flags, input.data(), static_cast<int>(input.size()),
                             &(*output)[0], count) == count;
}
} // namespace

std::string EncodeWindowsSaveName(const std::wstring& name) {
  std::string acp;
  std::wstring roundtrip;
  if (ConvertToBytes(CP_ACP, 0, name, &acp) &&
      ConvertToWide(CP_ACP, 0, acp, &roundtrip) && roundtrip == name)
    return acp;
  std::string utf8;
  if (!ConvertToBytes(CP_UTF8, kWideErrInvalidChars, name, &utf8)) return {};
  return kUtf8Tag + utf8;
}

bool DecodeWindowsSaveName(const std::string& encoded, std::wstring* name) {
  if (!name) return false;
  if (!encoded.empty() && encoded[0] == kUtf8Tag)
    return ConvertToWide(CP_UTF8, MB_ERR_INVALID_CHARS, encoded.substr(1), name);
  return ConvertToWide(CP_ACP, 0, encoded, name);
}

std::string EncodeWindowsProfileConfig(const std::wstring& name) {
  std::string utf8;
  if (!ConvertToBytes(CP_UTF8, kWideErrInvalidChars, name, &utf8)) return {};
  static const char hex[] = "0123456789ABCDEF";
  std::string encoded = "S2U8:";
  encoded.reserve(encoded.size() + utf8.size() * 2);
  for (unsigned char byte : utf8) {
    encoded += hex[byte >> 4];
    encoded += hex[byte & 15];
  }
  return encoded;
}

bool DecodeWindowsProfileConfig(const std::string& encoded, std::wstring* name) {
  if (!name) return false;
  if (encoded.compare(0, 5, "S2U8:") != 0)
    return ConvertToWide(CP_ACP, 0, encoded, name);
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
  return ConvertToWide(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, name);
}
} // namespace S2FileIO
