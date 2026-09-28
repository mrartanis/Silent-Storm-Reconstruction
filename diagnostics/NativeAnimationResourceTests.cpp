#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GAnimFormat.h"
#include "../FileIO/PortablePackageIndex.h"

#include <cstdint>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <set>

static std::uint64_t digest = UINT64_C(14695981039346656037);
static void Add(std::uint32_t value)
{
	for (int i = 0; i < 4; ++i)
	{
		digest ^= static_cast<std::uint8_t>(value >> (8 * i));
		digest *= UINT64_C(1099511628211);
	}
}
static void AddFloat(float value)
{
	std::uint32_t bits;
	std::memcpy(&bits, &value, sizeof(bits));
	Add(bits);
}
static void AddVec(const CVec3 &value)
{
	AddFloat(value.x); AddFloat(value.y); AddFloat(value.z);
}
static void AddQuat(const CQuat &value)
{
	float components[4];
	value.GetComponentsForWire(components);
	for (float component : components) AddFloat(component);
}
static void AddString(const std::string &value)
{
	Add(static_cast<std::uint32_t>(value.size()));
	for (unsigned char ch : value) Add(ch);
}
static bool OpenIndex(const std::filesystem::path &directory, const char *name,
	S2FileIO::PortablePackageIndex *index)
{
	return index->Open((directory / (std::string(name) + ".res")).string());
}
static std::set<std::int32_t> EffectiveIDs(const std::filesystem::path &directory,
	const char *name, const S2FileIO::PortablePackageIndex &index)
{
	std::set<std::int32_t> ids;
	for (const auto &entry : index.Entries()) ids.insert(entry.first);
	std::string resolved;
	if (S2FileIO::ResolveGameResourcePath((directory / name).string(), &resolved) &&
		std::filesystem::is_directory(resolved))
	{
		for (const auto &file : std::filesystem::directory_iterator(resolved))
		{
			if (!file.is_regular_file()) continue;
			const std::string fileName = file.path().filename().string();
			std::int32_t id = 0;
			const auto parsed = std::from_chars(fileName.data(),
				fileName.data() + fileName.size(), id);
			if (parsed.ec == std::errc() && parsed.ptr == fileName.data() + fileName.size())
				ids.insert(id);
		}
	}
	return ids;
}
static bool TestSkeletons(const std::filesystem::path &directory,
	const char *packageName, std::size_t *records, std::size_t *bones)
{
	S2FileIO::PortablePackageIndex index;
	if (!OpenIndex(directory, packageName, &index)) return false;
	const auto ids = EffectiveIDs(directory, packageName, index);
	AddString(packageName);
	for (std::int32_t id : ids)
	{
		try
		{
			NGScene::CResourceOpener file(packageName, id);
			CObj<NAnimation::CFileSkeletonInfo> value = new NAnimation::CFileSkeletonInfo;
			file->Add(1, &value->bones);
			if (std::strcmp(packageName, "Skeletons") == 0)
				file->Add(2, &value->bScale);
			if (std::strcmp(packageName, "Skeletons") == 0)
			{
				CObj<NAnimation::CFileSkeleton> loader = new NAnimation::CFileSkeleton;
				loader->SetKey(id);
				const auto *loaded = loader->GetValue();
				if (!loaded || loaded->bones.size() != value->bones.size() ||
					loaded->bScale != value->bScale) return false;
			}
			else
			{
				CObj<NAnimation::CFileLocators> loader = new NAnimation::CFileLocators;
				loader->SetKey(id);
				const auto *loaded = loader->GetValue();
				if (!loaded || loaded->bones.size() != value->bones.size()) return false;
			}
			Add(static_cast<std::uint32_t>(id));
			Add(static_cast<std::uint32_t>(value->bones.size()));
			if (std::strcmp(packageName, "Skeletons") == 0) Add(value->bScale ? 1 : 0);
			for (const auto &bone : value->bones)
			{
				AddString(bone.szName);
				Add(static_cast<std::uint32_t>(bone.nParent));
				AddVec(bone.pos); AddQuat(bone.rot); AddVec(bone.scale);
			}
			++*records;
			*bones += value->bones.size();
		}
		catch (...)
		{
			const auto packageEntry = index.Entries().find(id);
			std::fprintf(stderr, "%s decode failed id=%d package_bytes=%u\n",
				packageName, id, packageEntry == index.Entries().end() ? 0 : packageEntry->second.length);
			return false;
		}
	}
	std::printf("%s records=%zu bones=%zu cumulative_digest=%016llX\n",
		packageName, *records, *bones, static_cast<unsigned long long>(digest));
	return true;
}
static bool TestAnimations(const std::filesystem::path &directory,
	std::size_t *records, std::size_t *keys)
{
	S2FileIO::PortablePackageIndex index;
	if (!OpenIndex(directory, "Animations", &index)) return false;
	const auto ids = EffectiveIDs(directory, "Animations", index);
	AddString("Animations");
	for (std::int32_t id : ids)
		if (index.Entries().find(id) == index.Entries().end())
			std::printf("loose-only animation id=%d\n", id);
	for (std::int32_t id : ids)
	{
		try
		{
			NGScene::CResourceOpener file("Animations", id);
			CObj<NAnimation::CFileAnimationInfo> value = new NAnimation::CFileAnimationInfo;
			file->Add(1, &value->hdr);
			file->Add(2, &value->keysRoots);
			file->Add(3, &value->keysBones);
			file->Add(4, &value->keysAddBones);
			file->Add(5, &value->keysMSR);
			CObj<NAnimation::CFileAnimation> loader = new NAnimation::CFileAnimation;
			loader->SetKey(id);
			const auto *loaded = loader->GetValue();
			if (!loaded || loaded->hdr.indices.size() != value->hdr.indices.size() ||
				loaded->keysRoots.size() != value->keysRoots.size() ||
				loaded->keysBones.size() != value->keysBones.size() ||
				loaded->keysAddBones.size() != value->keysAddBones.size() ||
				loaded->keysMSR.size() != value->keysMSR.size()) return false;
			Add(static_cast<std::uint32_t>(id));
			const auto &hdr = value->hdr;
			Add(static_cast<std::uint32_t>(hdr.indices.size()));
			for (int indexValue : hdr.indices) Add(static_cast<std::uint32_t>(indexValue));
			AddFloat(hdr.fLength); AddFloat(hdr.fFrameRate);
			Add(static_cast<std::uint32_t>(hdr.nRoots));
			Add(static_cast<std::uint32_t>(hdr.nBones));
			Add(static_cast<std::uint32_t>(hdr.nAddBones));
			Add(hdr.bScale ? 1 : 0);
			Add(static_cast<std::uint32_t>(value->keysRoots.size()));
			for (const auto &key : value->keysRoots) { AddVec(key.pos); AddQuat(key.rot); }
			Add(static_cast<std::uint32_t>(value->keysBones.size()));
			for (const auto &key : value->keysBones) AddQuat(key.rot);
			Add(static_cast<std::uint32_t>(value->keysAddBones.size()));
			for (const auto &key : value->keysAddBones)
			{
				Add(static_cast<std::uint32_t>(key.nParent));
				AddVec(key.pos); AddQuat(key.rot);
			}
			Add(static_cast<std::uint32_t>(value->keysMSR.size()));
			for (const auto &key : value->keysMSR)
			{
				AddVec(key.pos); AddQuat(key.rot); AddVec(key.scale);
			}
			++*records;
			*keys += value->keysRoots.size() + value->keysBones.size() +
				value->keysAddBones.size() + value->keysMSR.size();
		}
		catch (...)
		{
			const auto packageEntry = index.Entries().find(id);
			std::fprintf(stderr, "Animations decode failed id=%d package_bytes=%u\n",
				id, packageEntry == index.Entries().end() ? 0 : packageEntry->second.length);
			return false;
		}
	}
	std::printf("Animations records=%zu keys=%zu cumulative_digest=%016llX\n",
		*records, *keys, static_cast<unsigned long long>(digest));
	return true;
}
int main(int argc, char **argv)
{
	if (argc != 2) return 2;
	const std::filesystem::path directory(argv[1]);
	NGScene::AddResourceDir(directory.string().c_str());
	std::size_t skeletons = 0, locators = 0, bones = 0, locatorBones = 0;
	std::size_t animations = 0, keys = 0;
	if (!TestSkeletons(directory, "Skeletons", &skeletons, &bones)) return 3;
	if (!TestSkeletons(directory, "Locators", &locators, &locatorBones)) return 4;
	if (!TestAnimations(directory, &animations, &keys)) return 5;
	NGScene::CloseAllResources();
	std::printf("skeletons=%zu bones=%zu locators=%zu locator_bones=%zu animations=%zu keys=%zu digest=%016llX\n",
		skeletons, bones, locators, locatorBones, animations, keys,
		static_cast<unsigned long long>(digest));
	// The effective shipping data includes 36 loose animation overrides and
	// two IDs present only as loose files, in addition to Animations.res.
	return skeletons == 139 && bones == 2080 && locators == 152 &&
		locatorBones == 152 && animations == 2857 && keys == 6734199 &&
		digest == UINT64_C(0x351FF3B89A911CE8) ? 0 : 6;
}
