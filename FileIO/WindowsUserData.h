#pragma once

#include <string>

namespace S2FileIO {

// Windows adapter around the portable path policy. An empty path means that
// the configured user-data directory could not be resolved or created.
const std::wstring& WindowsUserDataRoot();
std::wstring WindowsConfigPath();
std::wstring WindowsScreenshotDirectory();

} // namespace S2FileIO
