#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GAnimLight.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../FileIO/PortableStructureChunks.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>
#include <thread>

static std::uint64_t digest = UINT64_C(14695981039346656037);
template<class T> static void Value(T value)
{
	std::uint8_t bytes[sizeof(T)] = {};
	S2FileIO::EncodeStructureScalar(value, bytes, sizeof(bytes));
	for (std::uint8_t byte : bytes) {
		digest ^= byte;
		digest *= UINT64_C(1099511628211);
	}
}
static NGScene::CAnimLightInfo *Acquire(CDGPtr<NGScene::CLightLoader> &loader)
{
	const auto deadline = std::chrono::steady_clock::now() +
		std::chrono::seconds(30);
	while (std::chrono::steady_clock::now() < deadline) {
		MarkNewDGFrame();
		loader.Refresh();
		NGScene::CAnimLightInfo *info = loader->GetValue();
		if (info) return info;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return nullptr;
}
int main(int argc, char **argv)
{
	if (argc != 2) return 2;
	const std::filesystem::path packagePath(argv[1]);
	S2FileIO::PortablePackageIndex package;
	std::string error;
	if (!package.Open(packagePath.string(), &error)) return 3;
	NGScene::AddResourceDir(packagePath.parent_path().string().c_str());
	NGScene::RunResourceLoadingThread();
	std::uint64_t positionKeys = 0, colorKeys = 0, radiusKeys = 0;
	for (const auto &entry : package.Entries()) {
		CDGPtr<NGScene::CLightLoader> loader = new NGScene::CLightLoader;
		loader->SetKey(entry.first);
		NGScene::CAnimLightInfo *light = Acquire(loader);
		if (!light || !std::isfinite(light->fFrameRate) ||
			!std::isfinite(light->fTStart) || !std::isfinite(light->fTEnd) ||
			light->pos.keys.empty() || light->color.keys.empty() ||
			light->radius.keys.empty()) {
			std::printf("light %d unavailable or invalid\n", entry.first);
			return 4;
		}
		Value(static_cast<std::int32_t>(entry.first));
		Value(light->fFrameRate); Value(light->fTStart); Value(light->fTEnd);
		Value(static_cast<std::uint32_t>(light->pos.keys.size()));
		for (const auto &key : light->pos.keys) {
			Value(key.nT); Value(key.value.x); Value(key.value.y); Value(key.value.z);
		}
		Value(static_cast<std::uint32_t>(light->color.keys.size()));
		for (const auto &key : light->color.keys) {
			Value(key.nT); Value(key.value.x); Value(key.value.y); Value(key.value.z);
		}
		Value(static_cast<std::uint32_t>(light->radius.keys.size()));
		for (const auto &key : light->radius.keys) {
			Value(key.nT); Value(key.value);
		}
		positionKeys += light->pos.keys.size();
		colorKeys += light->color.keys.size();
		radiusKeys += light->radius.keys.size();
		CVec3 position, color;
		float radius;
		light->pos.GetValue(light->pos.keys.front().nT, &position);
		light->color.GetValue(light->color.keys.front().nT, &color);
		light->radius.GetValue(light->radius.keys.front().nT, &radius);
		if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
			!std::isfinite(position.z) || !std::isfinite(color.x) ||
			!std::isfinite(color.y) || !std::isfinite(color.z) ||
			!std::isfinite(radius)) return 5;
	}
	NGScene::CloseAllResources();
	std::printf("lights=%zu position_keys=%llu color_keys=%llu radius_keys=%llu digest=%016llX\n",
		package.Entries().size(), static_cast<unsigned long long>(positionKeys),
		static_cast<unsigned long long>(colorKeys),
		static_cast<unsigned long long>(radiusKeys),
		static_cast<unsigned long long>(digest));
	return package.Entries().size() == 21 ? 0 : 6;
}
