#pragma once
#include "NetworkSession.h"
#include <atomic>
namespace S2Net {
Compatibility IdentifyGameData(const std::vector<std::string>& resourceDirectories,
                              const std::vector<std::string>& databases,
                              const std::atomic<bool>& cancelled);
}
