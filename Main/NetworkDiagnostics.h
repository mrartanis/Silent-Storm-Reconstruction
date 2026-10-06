#pragma once
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif
namespace S2Net {
void NetworkLog(const std::string&) noexcept;
void StartupLog(const std::string&) noexcept;
void InstallNetworkCrashDiagnostics() noexcept;
#ifdef _WIN32
LONG WINAPI NetworkCrashFilter(EXCEPTION_POINTERS*);
#endif
}
