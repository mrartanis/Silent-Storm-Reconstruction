#if defined(_WIN32)
#include "../Main/StdAfx.h"
#include "../FileIO/WindowsUserData.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#include "../FileIO/LinuxUserData.h"
#endif
#include "../MiscDll/Commands.h"

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

int main()
{
	const std::filesystem::path path =
#if defined(_WIN32)
		S2FileIO::WindowsConfigPath();
#else
		S2FileIO::LinuxConfigPath();
	std::wstring decoded;
	if (S2FileIO::DecodeLinuxProfileConfig("S2U8:ZZ", &decoded) ||
		S2FileIO::DecodeLinuxProfileConfig("\xff", &decoded) ||
		!S2FileIO::DecodeLinuxProfileConfig("legacy", &decoded) ||
		decoded != L"legacy") return 6;
#endif
	const char* overrideRoot = std::getenv("S2_USER_DATA_DIR");
	if (path.empty() || path.filename() != "config.cfg" ||
		path.parent_path().filename() != "cfg" || !overrideRoot ||
		!std::filesystem::equivalent(path.parent_path().parent_path(),
			std::filesystem::path(overrideRoot))) return 1;

	NGlobal::RegisterVar("native_console_test", nullptr, nullptr,
		NGlobal::CValue(1.0f), true);
	NGlobal::RegisterVar("game_profile", nullptr, nullptr,
		NGlobal::CValue(std::wstring(L"Default")), true);
	NGlobal::SetVar("native_console_test", NGlobal::CValue(3.25f));
	NGlobal::SetVar("game_profile",
		NGlobal::CValue(std::wstring(L"\u0422\u0435\u0441\u0442")));
	NGlobal::SaveConfig(".\\cfg\\config.cfg");
	if (!std::filesystem::is_regular_file(path)) return 2;
	std::ifstream file(path, std::ios::binary);
	const std::string config((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	if (config.find("S2U8:") == std::string::npos ||
		config.find("native_console_test") == std::string::npos) return 5;
	NGlobal::ResetVar("native_console_test");
	NGlobal::ResetVar("game_profile");
	if (std::fabs(NGlobal::GetVar("native_console_test").GetFloat() - 1.0f) > 0.001f)
		return 3;
	NGlobal::LoadConfig(".\\cfg\\config.cfg");
	if (std::fabs(NGlobal::GetVar("native_console_test").GetFloat() - 3.25f) > 0.001f ||
		NGlobal::GetVar("game_profile").GetString() != L"\u0422\u0435\u0441\u0442")
		return 4;
	std::puts("native console config: passed");
	return 0;
}
