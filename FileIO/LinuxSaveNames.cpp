#include "WindowsSaveNames.h"
#include "LinuxUserData.h"

// The UI retains its shared save-name interface. Linux uses the existing UTF-8
// profile/slot codec also used by the native Linux save manager.
namespace S2FileIO {
std::string EncodeWindowsSaveName(const std::wstring& name) {
  return EncodeLinuxProfileConfig(name);
}
bool DecodeWindowsSaveName(const std::string& encoded, std::wstring* name) {
  return DecodeLinuxProfileConfig(encoded, name);
}
}
