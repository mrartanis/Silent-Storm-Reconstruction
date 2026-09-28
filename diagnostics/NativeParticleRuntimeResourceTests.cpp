#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GParticleFormat.h"
#include "../FileIO/PortableEffectData.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../FileIO/PortableStructureChunks.h"

#include <cstdint>
#include <chrono>
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
template<class T, class F>
static std::uint64_t Keys(const NGScene::TKeyTrack<T> &track, F add)
{
	if (track.nKeys < 0 || (track.nKeys && !track.keys)) return UINT64_MAX;
	Value(static_cast<std::uint32_t>(track.nKeys));
	for (int i = 0; i < track.nKeys; ++i) {
		Value(track.keys[i].nT);
		add(track.keys[i].value);
	}
	return static_cast<std::uint64_t>(track.nKeys);
}
static NGScene::CParticlesInfo *Acquire(CDGPtr<NGScene::CParticlesLoader> &loader)
{
	const auto deadline = std::chrono::steady_clock::now() +
		std::chrono::seconds(30);
	while (std::chrono::steady_clock::now() < deadline) {
		MarkNewDGFrame();
		loader.Refresh();
		NGScene::CParticlesInfo *info = loader->GetValue();
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
	std::uint64_t particles = 0, keys = 0;
	for (const auto &entry : package.Entries()) {
		const std::uint64_t before = digest;
		CDGPtr<NGScene::CParticlesLoader> loader = new NGScene::CParticlesLoader;
		loader->SetKey(entry.first);
		NGScene::CParticlesInfo *effect = Acquire(loader);
		if (!effect || effect->nParticles < 0 ||
			effect->particleStorage.size() != static_cast<std::size_t>(effect->nParticles) ||
			effect->keyStorage.size() != static_cast<std::size_t>(effect->nParticles) ||
			(effect->nParticles && effect->particles != effect->particleStorage.data())) {
			std::printf("effect %d unavailable or malformed\n", entry.first);
			return 4;
		}
		Value(static_cast<std::int32_t>(entry.first));
		Value(effect->fTEnd);
		Value(effect->fFrameRate);
		Value(static_cast<std::uint32_t>(effect->nParticles));
		particles += effect->nParticles;
		for (int i = 0; i < effect->nParticles; ++i) {
			const NGScene::SParticle &p = effect->particles[i];
			Value(p.nTStart); Value(p.nTEnd);
			std::uint64_t count = 0;
			count += Keys(p.pos, [](const CVec3 &v) { Value(v.x); Value(v.y); Value(v.z); });
			count += Keys(p.rot, [](float v) { Value(v); });
			count += Keys(p.scale, [](const CVec2 &v) { Value(v.x); Value(v.y); });
			count += Keys(p.color, [](DWORD v) { Value(static_cast<std::uint32_t>(v)); });
			count += Keys(p.sprite, [](short v) { Value(static_cast<std::int16_t>(v)); });
			if (count >= UINT64_MAX - keys) return 5;
			keys += count;
		}
		const std::uint64_t actual = digest;
		std::vector<std::uint8_t> bytes;
		S2FileIO::EffectData source;
		if (!package.Read(entry.first, &bytes, &error) ||
			!S2FileIO::DecodeEffectData(bytes.data(), bytes.size(), &source, &error))
			return 7;
		digest = before;
		Value(static_cast<std::int32_t>(entry.first));
		Value(source.endTime); Value(source.frameRate);
		Value(static_cast<std::uint32_t>(source.particles.size()));
		for (const auto &p : source.particles) {
			Value(p.start); Value(p.end);
			Value(static_cast<std::uint32_t>(p.position.size()));
			for (const auto &k : p.position) {
				Value(k.frame); Value(k.value.x); Value(k.value.y); Value(k.value.z);
			}
			Value(static_cast<std::uint32_t>(p.rotation.size()));
			for (const auto &k : p.rotation) { Value(k.frame); Value(k.value); }
			Value(static_cast<std::uint32_t>(p.scale.size()));
			for (const auto &k : p.scale) {
				Value(k.frame); Value(k.value.x); Value(k.value.y);
			}
			Value(static_cast<std::uint32_t>(p.color.size()));
			for (const auto &k : p.color) { Value(k.frame); Value(k.value); }
			Value(static_cast<std::uint32_t>(p.sprite.size()));
			for (const auto &k : p.sprite) { Value(k.frame); Value(k.value); }
		}
		if (digest != actual) {
			std::printf("effect %d mismatch runtime=%016llX parser=%016llX\n",
				entry.first, static_cast<unsigned long long>(actual),
				static_cast<unsigned long long>(digest));
			return 8;
		}
		digest = actual;
	}
	NGScene::CloseAllResources();
	std::printf("effects=%zu particles=%llu keys=%llu fnv64=%016llX\n",
		package.Entries().size(), static_cast<unsigned long long>(particles),
		static_cast<unsigned long long>(keys),
		static_cast<unsigned long long>(digest));
	return package.Entries().size() == 280 && particles == 94612 &&
		keys == 1917803 && digest == UINT64_C(0x89E2A0AF09D801B5) ? 0 : 6;
}
