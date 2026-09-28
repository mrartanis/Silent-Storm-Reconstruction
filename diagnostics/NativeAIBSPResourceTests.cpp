#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/aiObjectLoader.h"
#include "../FileIO/PortablePackageIndex.h"

#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <set>

static std::uint64_t digest = UINT64_C(14695981039346656037);
static void Add(std::uint32_t value)
{
	for (int byte = 0; byte < 4; ++byte)
	{
		digest ^= static_cast<std::uint8_t>(value >> (byte * 8));
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
static void AddPrecalc(const NAI::CPrecalcSpheres &value, std::size_t *cells)
{
	const auto &grid = value.isCollided;
	Add(grid.GetXSize()); Add(grid.GetYSize());
	for (int y = 0; y < grid.GetYSize(); ++y)
		for (int x = 0; x < grid.GetXSize(); ++x)
		{
			Add(static_cast<std::uint32_t>(grid[y][x]));
			++*cells;
		}
	AddVec(value.bound.s.ptCenter); AddFloat(value.bound.s.fRadius);
	AddVec(value.bound.ptHalfBox); AddVec(value.ptMin); AddVec(value.ptMax);
}
static void AddStages(const vector<NAI::CPrecalcPieces> &stages,
	std::size_t *pieces, std::size_t *cells)
{
	Add(static_cast<std::uint32_t>(stages.size()));
	for (const auto &stage : stages)
	{
		std::map<int, CPtr<NAI::CPrecalcSpheres>> ordered(stage.begin(), stage.end());
		Add(static_cast<std::uint32_t>(ordered.size()));
		for (const auto &piece : ordered)
		{
			Add(static_cast<std::uint32_t>(piece.first));
			Add(IsValid(piece.second) ? 1 : 0);
			if (IsValid(piece.second)) AddPrecalc(*piece.second, cells);
			++*pieces;
		}
	}
}
static std::set<std::int32_t> EffectiveIDs(const std::filesystem::path &directory,
	const S2FileIO::PortablePackageIndex &index, std::set<std::int32_t> *looseIDs)
{
	std::set<std::int32_t> ids;
	for (const auto &entry : index.Entries()) ids.insert(entry.first);
	std::string looseDir;
	if (S2FileIO::ResolveGameResourcePath((directory / "AIBSPTrees").string(), &looseDir) &&
		std::filesystem::is_directory(looseDir))
		for (const auto &file : std::filesystem::directory_iterator(looseDir))
		{
			if (!file.is_regular_file()) continue;
			const std::string name = file.path().filename().string();
			std::int32_t id = 0;
			const auto parsed = std::from_chars(name.data(), name.data() + name.size(), id);
			if (parsed.ec == std::errc() && parsed.ptr == name.data() + name.size())
			{
				ids.insert(id);
				looseIDs->insert(id);
			}
		}
	return ids;
}
int main(int argc, char **argv)
{
	if (argc != 2) return 2;
	const std::filesystem::path directory(argv[1]);
	S2FileIO::PortablePackageIndex index;
	if (!index.Open((directory / "AIBSPTrees.res").string())) return 3;
	NGScene::AddResourceDir(directory.string().c_str());
	std::set<std::int32_t> looseIDs;
	const auto ids = EffectiveIDs(directory, index, &looseIDs);
	std::size_t records = 0, openStages = 0, closedStages = 0;
	std::size_t pieces = 0, cells = 0, overrides = 0, looseOnly = 0;
	for (std::int32_t id : ids)
	{
		if (looseIDs.count(id)) ++overrides;
		if (index.Entries().find(id) == index.Entries().end()) ++looseOnly;
		try
		{
			NGScene::CResourceOpener file("AIBSPTrees", id);
			vector<NAI::CPrecalcPieces> open, closed;
			file->Add(3, &open); file->Add(4, &closed);
			CObj<NAI::CLoadTwoBSPTrees> loader = new NAI::CLoadTwoBSPTrees;
			loader->SetKey(id);
			const auto *loaded = loader->GetValue();
			if (!loaded || loaded->treesOpen.size() != open.size() ||
				loaded->treesClosed.size() != closed.size())
			{
				std::fprintf(stderr, "AIBSPTrees loader mismatch id=%d\n", id);
				return 4;
			}
			Add(static_cast<std::uint32_t>(id));
			AddStages(open, &pieces, &cells);
			AddStages(closed, &pieces, &cells);
			openStages += open.size(); closedStages += closed.size(); ++records;
		}
		catch (const std::exception &error)
		{
			std::fprintf(stderr, "AIBSPTrees decode failed id=%d error=%s\n", id, error.what());
			return 5;
		}
		catch (...)
		{
			std::fprintf(stderr, "AIBSPTrees decode failed id=%d\n", id);
			return 5;
		}
	}
	NGScene::CloseAllResources();
	std::printf("records=%zu loose_files=%zu overrides=%zu loose_only=%zu open_stages=%zu closed_stages=%zu pieces=%zu cells=%zu digest=%016llX\n",
		records, looseIDs.size(), overrides, looseOnly, openStages,
		closedStages, pieces, cells, static_cast<unsigned long long>(digest));
	return records == 193 && looseIDs.size() == 193 && overrides == 193 &&
		looseOnly == 0 && openStages == 965 && closedStages == 965 &&
		pieces == 1006 && cells == 265169 &&
		digest == UINT64_C(0x4D794836A2553C87) ? 0 : 6;
}
