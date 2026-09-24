#include "../FileIO/PortableUserPaths.h"

#include <cstdio>
#include <string>

using namespace S2FileIO;
#define CHECK(expression) do { if (!(expression)) { \
  std::fprintf(stderr, "failed: %s at line %d\n", #expression, __LINE__); return 1; \
} } while (0)

int main() {
  std::string root, error;
  UserPathEnvironment win;
  win.localAppData = "C:/Users/Test/AppData/Local/";
  CHECK(ResolveUserDataRoot(HostPlatform::Windows, win, &root, &error));
  CHECK(root == "C:\\Users\\Test\\AppData\\Local\\Silent Storm Reconstruction");
  win.overrideRoot = "G:/Runs/one/user-data/";
  CHECK(ResolveUserDataRoot(HostPlatform::Windows, win, &root, &error));
  CHECK(root == "G:\\Runs\\one\\user-data");
  win.overrideRoot = "..\\save";
  CHECK(!ResolveUserDataRoot(HostPlatform::Windows, win, &root, &error));
  win.overrideRoot = "C:\\data\\..\\save";
  CHECK(!ResolveUserDataRoot(HostPlatform::Windows, win, &root, &error));

  UserPathEnvironment posix;
  posix.home = "/home/player";
  CHECK(ResolveUserDataRoot(HostPlatform::Linux, posix, &root, &error));
  CHECK(root == "/home/player/.local/share/silent-storm-reconstruction");
  posix.xdgDataHome = "/mnt/user-data";
  CHECK(ResolveUserDataRoot(HostPlatform::Linux, posix, &root, &error));
  CHECK(root == "/mnt/user-data/silent-storm-reconstruction");
  CHECK(ResolveUserDataRoot(HostPlatform::MacOS, posix, &root, &error));
  CHECK(root == "/home/player/Library/Application Support/Silent Storm Reconstruction");
  posix.overrideRoot = "relative/path";
  CHECK(!ResolveUserDataRoot(HostPlatform::Linux, posix, &root, &error));

  CHECK(IsSafeSaveComponent("stational weapons"));
  CHECK(IsSafeSaveComponent("Сохранение"));
  for (const char* bad : {"", ".", "..", "../outside", "a\\b", "C:", "name.", "name ", "a?b", "NUL", "con.txt", "COM1"})
    CHECK(!IsSafeSaveComponent(bad));

  WideUserPathEnvironment wide;
  wide.localAppData = L"C:/Users/Игрок/AppData/Local/";
  std::wstring wideRoot;
  CHECK(ResolveUserDataRoot(HostPlatform::Windows, wide, &wideRoot, &error));
  CHECK(wideRoot == L"C:\\Users\\Игрок\\AppData\\Local\\Silent Storm Reconstruction");
  wide.overrideRoot = L"G:/Тестовые данные/";
  CHECK(ResolveUserDataRoot(HostPlatform::Windows, wide, &wideRoot, &error));
  CHECK(wideRoot == L"G:\\Тестовые данные");
  CHECK(IsSafeSaveComponent(std::wstring(L"Сохранение 1")));
  CHECK(!IsSafeSaveComponent(std::wstring(L"..\\выход")));
}
