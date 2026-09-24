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

// Produces an absolute user-writable root, independently of the process cwd.
// The caller supplies environment values so the policy is testable on every host.
bool ResolveUserDataRoot(HostPlatform platform, const UserPathEnvironment& env,
                         std::string* result, std::string* error = nullptr);
bool IsSafeSaveComponent(const std::string& component);

} // namespace S2FileIO
