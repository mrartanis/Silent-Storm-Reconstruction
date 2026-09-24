#pragma once

#include <string>

namespace S2FileIO {

enum class HostPlatform { Windows, Linux, MacOS };

struct UserPathEnvironment {
  std::string overrideRoot;
  std::string localAppData;
  std::string home;
  std::string xdgDataHome;
};

struct WideUserPathEnvironment {
  std::wstring overrideRoot;
  std::wstring localAppData;
  std::wstring home;
  std::wstring xdgDataHome;
};

// Produces an absolute user-writable root, independently of the process cwd.
// The caller supplies environment values so the policy is testable on every host.
bool ResolveUserDataRoot(HostPlatform platform, const UserPathEnvironment& env,
                         std::string* result, std::string* error = nullptr);
bool ResolveUserDataRoot(HostPlatform platform, const WideUserPathEnvironment& env,
                         std::wstring* result, std::string* error = nullptr);
bool IsSafeSaveComponent(const std::string& component);
bool IsSafeSaveComponent(const std::wstring& component);

} // namespace S2FileIO
