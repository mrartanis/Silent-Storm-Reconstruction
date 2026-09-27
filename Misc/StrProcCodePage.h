#pragma once

#include <string>

namespace NStr {
// Convert a wide game.db field using an explicit legacy code page without
// changing the process-wide text code page used by unrelated game paths.
void ToAsciiCodePage(std::string* output, const std::wstring& input, int codePage);
}
