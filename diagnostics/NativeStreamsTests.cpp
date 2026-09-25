#include "../FileIO/StdAfx.h"
#include "../FileIO/Streams.h"

#include <chrono>
#include <codecvt>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <locale>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  try {
    CMemoryStream memory;
    memory.WriteString("short");
    memory.WriteString(std::string(300, 'x'));
    memory.Seek(0);
    std::string shortText, longText;
    memory.ReadString(shortText);
    memory.ReadString(longText);
    if (shortText != "short" || longText != std::string(300, 'x')) return 1;

    const std::vector<std::uint8_t> payload(4096, 0xa5);
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::wstring filename = std::wstring(L"s2-streams-") +
        std::to_wstring(stamp) + L"-тест.bin";
#ifdef _WIN32
    const std::filesystem::path file = std::filesystem::temp_directory_path() / filename;
    const std::wstring wideFile = file.wstring();
#else
    const std::string directory = std::filesystem::temp_directory_path().string();
    const std::wstring wideFile = std::wstring(directory.begin(), directory.end()) +
        L"/" + filename;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8;
    const std::filesystem::path file = utf8.to_bytes(wideFile);
#endif
    {
      CFileStream stream;
      stream.OpenWrite(wideFile.c_str());
      stream.Write(payload.data(), static_cast<unsigned>(payload.size()));
      stream.CloseFile();
    }
    {
      CFileStream stream;
      stream.OpenRead(wideFile.c_str());
      if (stream.GetSize() != static_cast<int>(payload.size())) return 2;
      std::vector<std::uint8_t> bytes(payload.size());
      stream.Read(bytes.data(), static_cast<unsigned>(bytes.size()));
      if (bytes != payload) return 3;
      stream.Seek(1017);
      std::uint8_t across[32] = {};
      stream.Read(across, sizeof(across));
      for (std::uint8_t value : across)
        if (value != 0xa5) return 4;
      stream.CloseFile();
    }
    std::filesystem::remove(file);
    if (argc == 2) {
      std::ifstream reference(argv[1], std::ios::binary);
      if (!reference) return 6;
      const std::vector<std::uint8_t> expected(
          (std::istreambuf_iterator<char>(reference)),
          std::istreambuf_iterator<char>());
      if (expected.empty()) return 7;
      CFileStream original;
      original.OpenRead(argv[1]);
      if (original.GetSize() != static_cast<int>(expected.size())) return 8;
      std::vector<std::uint8_t> actual(expected.size());
      original.Read(actual.data(), static_cast<unsigned>(actual.size()));
      if (actual != expected) return 9;
      std::uint64_t hash = UINT64_C(14695981039346656037);
      for (std::uint8_t value : actual)
        hash = (hash ^ value) * UINT64_C(1099511628211);
      std::printf("native-stream original-data bytes %zu fnv64 %016llx\n",
          actual.size(), static_cast<unsigned long long>(hash));
    } else if (argc != 1) {
      return 10;
    }
    return 0;
  } catch (const SFileIOError& error) {
    std::fprintf(stderr, "NativeStreamsTests: %s\n", error.szError.c_str());
  } catch (const std::exception& error) {
    std::fprintf(stderr, "NativeStreamsTests: %s\n", error.what());
  }
  return 5;
}
