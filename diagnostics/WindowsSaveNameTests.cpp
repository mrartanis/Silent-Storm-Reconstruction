#include "../FileIO/WindowsSaveNames.h"

#include <string>

int main() {
  const std::wstring names[] = {L"DB_OLD", L"Сохранение 1", L"漢字", L"\U0001f680 test"};
  for (const std::wstring& original : names) {
    const std::string encoded = S2FileIO::EncodeWindowsSaveName(original);
    std::wstring restored;
    if (encoded.empty() || !S2FileIO::DecodeWindowsSaveName(encoded, &restored) ||
        restored != original) return 1;
  }
  if (S2FileIO::EncodeWindowsSaveName(L"DB_OLD") != "DB_OLD") return 2;
  std::wstring restored;
  if (S2FileIO::DecodeWindowsSaveName(std::string("\x1f\xff"), &restored) ||
      S2FileIO::DecodeWindowsSaveName("anything", nullptr)) return 3;
  const std::wstring profile = L"Профиль 漢字";
  const std::string config = S2FileIO::EncodeWindowsProfileConfig(profile);
  if (config.compare(0, 5, "S2U8:") != 0 ||
      config.find(' ') != std::string::npos ||
      !S2FileIO::DecodeWindowsProfileConfig(config, &restored) || restored != profile ||
      !S2FileIO::DecodeWindowsProfileConfig("default", &restored) || restored != L"default" ||
      S2FileIO::DecodeWindowsProfileConfig("S2U8:FF", &restored) ||
      S2FileIO::DecodeWindowsProfileConfig("S2U8:A", &restored) ||
      S2FileIO::DecodeWindowsProfileConfig("S2U8:XX", &restored)) return 4;
  return 0;
}
