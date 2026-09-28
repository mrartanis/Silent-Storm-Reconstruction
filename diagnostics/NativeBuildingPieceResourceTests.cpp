#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/PortableMeshCodecs.h"
#include "../Main/GObjectInfoLoadCore.h"
#include "../FileIO/PortableGameDatabase.h"
#include "../FileIO/PortablePackageIndex.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <unordered_set>

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
static int Column(const std::vector<std::string> &names, const char *name)
{
	auto it = std::find(names.begin(), names.end(), name);
	return it == names.end() ? -1 : static_cast<int>(it - names.begin());
}

int main(int argc, char **argv)
{
	if (argc != 3) return 2;
	S2FileIO::PortableGameDatabase database;
	std::string error;
	if (!S2FileIO::LoadPortableGameDatabase(argv[1], &database, &error))
	{
		std::fprintf(stderr, "game.db: %s\n", error.c_str());
		return 3;
	}
	std::unordered_set<int> geometryIDs;
	std::size_t constructionRows = 0;
	for (const auto &table : database.tables)
	{
		if (table.tableId != 47) continue; // ConstructionParts
		const int first = Column(table.intNames, "FirstGeometryID");
		const int second = Column(table.intNames, "SecondGeometryID");
		if (first < 0 || second < 0) return 4;
		for (const auto &row : table.intRows)
		{
			++constructionRows;
			if (row[first] > 0) geometryIDs.insert(row[first]);
			if (row[second] > 0) geometryIDs.insert(row[second]);
		}
	}
	if (!constructionRows || geometryIDs.empty()) return 5;
	// CBuilding::Build obtains each wall/solid geometry through its DB pointer.
	// This is a separate Geometries.res loader from CGameView's model path,
	// but it cannot reach package IDs whose low 16 bits lack a CGeometry row.
	std::unordered_set<int> runtimeGeometries;
	for (const auto &table : database.tables)
	{
		if (table.tableId != 10) continue; // Geometries
		const int idColumn = Column(table.intNames, "ID");
		if (idColumn < 0) return 12;
		for (const auto &row : table.intRows)
			runtimeGeometries.insert(row[idColumn]);
	}
	if (runtimeGeometries.empty()) return 12;
	for (int geometryID : geometryIDs)
		if (!runtimeGeometries.count(geometryID)) return 13;
	const std::filesystem::path resourceDir(argv[2]);
	S2FileIO::PortablePackageIndex index;
	if (!index.Open((resourceDir / "Geometries.res").string())) return 6;
	NGScene::AddResourceDir(resourceDir.string().c_str());
	std::size_t entries = 0, nonempty = 0, parts = 0, unlinked = 0;
	std::size_t vertices = 0, indices = 0, polygons = 0;
	std::vector<int> emptyIDs;
	for (const auto &entry : index.Entries())
	{
		const int id = entry.first;
		if (!runtimeGeometries.count(id & 0xffff)) ++unlinked;
		if (!geometryIDs.count(id & 0xffff)) continue;
		try
		{
			NGScene::CResourceOpener file("Geometries", id);
			CPtr<NGScene::CObjectInfoPieces> piece = new NGScene::CObjectInfoPieces;
			NGScene::ReadObjectInfoPieces(file.operator->(), piece);
			++entries;
			if (!piece->faces.empty()) ++nonempty;
			else emptyIDs.push_back(id);
			Add(static_cast<std::uint32_t>(id));
			Add(static_cast<std::uint32_t>(piece->faces.size()));
			std::vector<int> partIDs;
			partIDs.reserve(piece->faces.size());
			for (const auto &face : piece->faces) partIDs.push_back(face.first);
			std::sort(partIDs.begin(), partIDs.end());
			for (int partID : partIDs)
			{
				const auto &face = piece->faces.at(partID);
				++parts;
				vertices += face.points.size();
				indices += face.polys.indices.size();
				polygons += face.polys.polys.size();
				Add(static_cast<std::uint32_t>(partID));
				Add(static_cast<std::uint32_t>(face.points.size()));
				for (const auto &point : face.points)
				{
					AddFloat(point.pos.x); AddFloat(point.pos.y); AddFloat(point.pos.z);
					AddFloat(point.normal.x); AddFloat(point.normal.y); AddFloat(point.normal.z);
					AddFloat(point.tex.x); AddFloat(point.tex.y);
					AddFloat(point.texU.x); AddFloat(point.texU.y); AddFloat(point.texU.z);
					AddFloat(point.texV.x); AddFloat(point.texV.y); AddFloat(point.texV.z);
					Add(point.dwColor);
				}
				Add(static_cast<std::uint32_t>(face.polys.indices.size()));
				for (WORD indexValue : face.polys.indices)
				{
					if (indexValue >= face.points.size()) return 7;
					Add(indexValue);
				}
				Add(static_cast<std::uint32_t>(face.polys.polys.size()));
				WORD previous = 0;
				for (WORD end : face.polys.polys)
				{
					if (end < previous || end > face.polys.indices.size()) return 8;
					Add(end);
					previous = end;
				}
				if (!face.polys.polys.empty() &&
					(face.polys.polys.front() != 0 || previous != face.polys.indices.size()))
					return 9;
			}
		}
		catch (...)
		{
			std::fprintf(stderr, "Building geometry pieces failed id=%d\n", id);
			return 10;
		}
	}
	NGScene::CloseAllResources();
	std::printf("construction_rows=%zu geometry_ids=%zu entries=%zu nonempty=%zu parts=%zu vertices=%zu indices=%zu polygons=%zu digest=%016llX\n",
		constructionRows, geometryIDs.size(), entries, nonempty, parts,
		vertices, indices, polygons, static_cast<unsigned long long>(digest));
	std::printf("empty_ids=");
	for (std::size_t i = 0; i < emptyIDs.size(); ++i)
		std::printf("%s%d", i ? "," : "", emptyIDs[i]);
	std::printf("\n");
	std::printf("unlinked_package_entries=%zu\n", unlinked);
	return constructionRows == 885 && geometryIDs.size() == 814 &&
		entries == 2928 && nonempty == 2926 && parts == 28188 &&
		vertices == 853549 && indices == 1049304 && polygons == 294617 &&
		unlinked == 966 && digest == UINT64_C(0x288D6D988119C2E5) ? 0 : 11;
}
