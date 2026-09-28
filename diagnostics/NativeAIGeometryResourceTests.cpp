#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/aiObjectLoader.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../FileIO/Streams.h"
#include "../DBFormat/DataGeometry.h"
#include "../DBFormat/DataFormat.h"
#include "../DBFormat/DataMap.h"
#include "../DBFormat/DataObject.h"
#include "../DBFormat/DataScenario.h"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <set>

// Same original AIGeometries disk fields as CLoadGeometryInfo::Recalc.
struct SStoredPieceProbe
{
	vector<STriangle> tris;
	vector<CVec3> verts;
	vector<NGScene::SLoadVertexWeight> weights;
	float fVolume = 0;
	vector<NAI::SJunction> juncs;
	vector<CPtr<NAI::CPrecalcSpheres>> precalc;
	int operator&(CStructureSaver &f)
	{
		f.Add(1, &verts); f.Add(2, &tris); f.Add(3, &weights);
		f.Add(10, &fVolume); f.Add(11, &juncs); f.Add(13, &precalc);
		return 0;
	}
};
using StoredPieces = unordered_map<int, SStoredPieceProbe>;
static std::uint64_t digest = UINT64_C(14695981039346656037);
static void Add(std::uint32_t value)
{
	for (int i = 0; i < 4; ++i)
	{
		digest ^= static_cast<std::uint8_t>(value >> (i * 8));
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
static void AddTriangles(const vector<STriangle> &tris)
{
	Add(static_cast<std::uint32_t>(tris.size()));
	for (const auto &tri : tris)
	{ Add(tri.i1); Add(tri.i2); Add(tri.i3); }
}
static void AddPrecalc(const vector<CPtr<NAI::CPrecalcSpheres>> &values,
	std::size_t *gridCells)
{
	Add(static_cast<std::uint32_t>(values.size()));
	for (const auto &value : values)
	{
		Add(IsValid(value) ? 1 : 0);
		if (!IsValid(value)) continue;
		const auto &grid = value->isCollided;
		Add(grid.GetXSize()); Add(grid.GetYSize());
		for (int y = 0; y < grid.GetYSize(); ++y)
			for (int x = 0; x < grid.GetXSize(); ++x)
			{
				Add(static_cast<std::uint32_t>(grid[y][x]));
				++*gridCells;
			}
		AddVec(value->bound.s.ptCenter); AddFloat(value->bound.s.fRadius);
		AddVec(value->bound.ptHalfBox);
		AddVec(value->ptMin); AddVec(value->ptMax);
	}
}
static std::set<std::int32_t> EffectiveIDs(const std::filesystem::path &directory,
	const S2FileIO::PortablePackageIndex &index, std::set<std::int32_t> *looseIDs)
{
	std::set<std::int32_t> ids;
	for (const auto &entry : index.Entries()) ids.insert(entry.first);
	std::string looseDir;
	if (S2FileIO::ResolveGameResourcePath((directory / "AIGeometries").string(), &looseDir) &&
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
static std::uint64_t HashIDs(const std::set<std::int32_t> &ids)
{
	std::uint64_t hash = UINT64_C(14695981039346656037);
	for (std::int32_t id : ids)
		for (int byte = 0; byte < 4; ++byte)
		{
			hash ^= static_cast<std::uint8_t>(id >> (byte * 8));
			hash *= UINT64_C(1099511628211);
		}
	return hash;
}
static std::set<int> ScenarioVariantIDs()
{
	auto *zones = NDatabase::GetTable<NDb::CDBScenarioZone>();
	auto *templates = NDatabase::GetTable<NDb::CTemplate>();
	if (!zones || !templates) throw 1;
	std::set<int> seenTemplates, variantIDs;
	std::vector<int> pending;
	CDBIterator<NDb::CDBScenarioZone> zone(*zones);
	while (zone.MoveNext())
		for (int id : zone.Get()->templatesIDs)
			if (id > 0) pending.push_back(id);
	while (!pending.empty())
	{
		int id = pending.back(); pending.pop_back();
		if (!seenTemplates.insert(id).second) continue;
		auto *map = static_cast<NDb::CTemplate *>(templates->GetDBRecord(id));
		if (!map) throw 2;
		for (const auto &variant : map->variants)
		{
			if (!variant) throw 3;
			variantIDs.insert(variant->GetRecordID());
			for (const auto &rect : variant->rects)
				if (rect && rect->pTemplate)
					pending.push_back(rect->pTemplate->GetRecordID());
		}
	}
	return variantIDs;
}
static int CheckCollisionRoots(const std::filesystem::path &directory,
	const std::set<std::int32_t> &available,
	const std::set<std::int32_t> &geometryIDs)
{
	S2FileIO::PortablePackageIndex binds, doors;
	if (!binds.Open((directory / "AIBinds.res").string()) ||
		!doors.Open((directory / "AIBSPTrees.res").string())) return 8;
	auto *doorTable = NDatabase::GetTable<NDb::CDoor>();
	if (!doorTable) return 9;
	std::set<std::int32_t> doorIDs, bindIDs, treeIDs, extraGeometry;
	CDBIterator<NDb::CDoor> door(*doorTable);
	while (door.MoveNext()) doorIDs.insert(door.Get()->GetRecordID());
	for (const auto &entry : binds.Entries()) bindIDs.insert(entry.first);
	for (const auto &entry : doors.Entries()) treeIDs.insert(entry.first);
	std::set_difference(available.begin(), available.end(),
		geometryIDs.begin(), geometryIDs.end(),
		std::inserter(extraGeometry, extraGeometry.end()));
	std::size_t bindsInGeometry = 0, treesInDoor = 0;
	for (int id : bindIDs) bindsInGeometry += geometryIDs.count(id);
	for (int id : treeIDs) treesInDoor += doorIDs.count(id);
	std::set<std::int32_t> extraBinds, doorsWithoutTrees;
	std::set_difference(bindIDs.begin(), bindIDs.end(),
		geometryIDs.begin(), geometryIDs.end(),
		std::inserter(extraBinds, extraBinds.end()));
	std::set_difference(doorIDs.begin(), doorIDs.end(),
		treeIDs.begin(), treeIDs.end(),
		std::inserter(doorsWithoutTrees, doorsWithoutTrees.end()));
	std::printf("collision_roots ai_geometry_db=%zu available=%zu extra_package=%zu extra_digest=%016llX ai_binds=%zu binds_in_geometry_db=%zu door_db=%zu bsp_trees=%zu trees_in_door_db=%zu door_digest=%016llX\n",
		geometryIDs.size(), available.size(), extraGeometry.size(),
		static_cast<unsigned long long>(HashIDs(extraGeometry)), bindIDs.size(),
		bindsInGeometry, doorIDs.size(), treeIDs.size(), treesInDoor,
		static_cast<unsigned long long>(HashIDs(doorIDs)));
	for (int id : extraGeometry)
		std::printf("collision_extra_ai_geometry id=%d\n", id);
	for (int id : extraBinds)
		std::printf("collision_extra_ai_bind id=%d\n", id);
	std::printf("collision_extra_ai_bind_digest=%016llX\n",
		static_cast<unsigned long long>(HashIDs(extraBinds)));
	for (int id : doorsWithoutTrees)
		std::printf("collision_door_without_bsp id=%d\n", id);
	const auto scenarioVariants = ScenarioVariantIDs();
	auto *variants = NDatabase::GetTable<NDb::CTemplVariant>();
	if (!variants) return 11;
	std::set<int> missingDoorFinalElements, missingDoorScenarioVariants;
	CDBIterator<NDb::CTemplVariant> variant(*variants);
	while (variant.MoveNext())
		for (const auto &element : variant.Get()->pFinalElements)
			if (element && element->pObject && element->pObject->pObject &&
				element->pObject->pObject->pDoor &&
				element->pObject->pObject->pDoor->GetRecordID() == 234)
			{
				missingDoorFinalElements.insert(element->GetRecordID());
				if (scenarioVariants.count(variant.Get()->GetRecordID()))
					missingDoorScenarioVariants.insert(variant.Get()->GetRecordID());
			}
	std::printf("collision_missing_bsp_door_234 scenario_variants=%zu all_final_elements=%zu scenario_hits=%zu hit_digest=%016llX\n",
		scenarioVariants.size(), missingDoorFinalElements.size(), missingDoorScenarioVariants.size(),
		static_cast<unsigned long long>(HashIDs(missingDoorScenarioVariants)));
	for (int id : missingDoorScenarioVariants)
		std::printf("collision_missing_bsp_door_234 scenario_variant=%d\n", id);
	return geometryIDs.size() == 1982 && available.size() == 2043 &&
		extraGeometry.size() == 61 && HashIDs(extraGeometry) == UINT64_C(0x9D77FA3BE8DF0C85) &&
		bindIDs.size() == 211 && bindsInGeometry == 205 &&
		extraBinds.size() == 6 && HashIDs(extraBinds) == UINT64_C(0xAC396197B6ABB244) &&
		doorIDs.size() == 194 && treeIDs.size() == 193 && treesInDoor == 193 &&
		HashIDs(doorIDs) == UINT64_C(0x622781497A448A20) &&
		scenarioVariants.size() == 985 && missingDoorFinalElements.size() == 7 &&
		missingDoorScenarioVariants.size() == 4 &&
		HashIDs(missingDoorScenarioVariants) == UINT64_C(0xE5A8E95E5F10ACE1) ? 0 : 10;
}
int main(int argc, char **argv)
{
	if (argc != 3 && !(argc == 4 && std::strcmp(argv[3], "--collision-roots") == 0)) return 2;
	CFileStream database;
	database.OpenRead(argv[1]);
	NDatabase::Serialize(database, CStructureSaver::READ);
	auto *geometryTable = NDatabase::GetTable<NDb::CAIGeometry>();
	if (!geometryTable) return 7;
	const std::filesystem::path directory(argv[2]);
	S2FileIO::PortablePackageIndex index;
	if (!index.Open((directory / "AIGeometries.res").string())) return 3;
	NGScene::AddResourceDir(directory.string().c_str());
	std::set<std::int32_t> looseIDs;
	const auto available = EffectiveIDs(directory, index, &looseIDs);
	std::set<std::int32_t> ids;
	CDBIterator<NDb::CAIGeometry> geometry(*geometryTable);
	while (geometry.MoveNext())
	{
		const int id = geometry.Get()->GetRecordID();
		if (available.count(id)) ids.insert(id);
	}
	if (argc == 4) return CheckCollisionRoots(directory, available, ids);
	std::size_t geometries = 0, points = 0, triangles = 0, spheres = 0;
	std::size_t piecesTotal = 0, precalcTotal = 0, gridCells = 0;
	std::size_t looseOnly = 0, overrides = 0;
	for (std::int32_t id : ids)
	{
		if (index.Entries().find(id) == index.Entries().end()) ++looseOnly;
		if (looseIDs.count(id)) ++overrides;
		const char *phase = "open";
		try
		{
			NGScene::CResourceOpener file("AIGeometries", id);
			vector<CVec3> rawPoints;
			vector<STriangle> rawTriangles;
			StoredPieces pieces;
			vector<SMassSphere> rawSpheres;
			vector<CPtr<NAI::CPrecalcSpheres>> precalc;
			phase = "points"; file->Add(1, &rawPoints);
			phase = "triangles"; file->Add(2, &rawTriangles);
			phase = "pieces"; file->Add(4, &pieces);
			phase = "spheres"; file->Add(6, &rawSpheres);
			phase = "precalc"; file->Add(9, &precalc);
			phase = "loader";
			CObj<NAI::CLoadGeometryInfo> loader = new NAI::CLoadGeometryInfo;
			loader->SetKey(id);
			const auto *loaded = loader->GetValue();
			if (!loaded || loaded->pieces.size() != (pieces.empty() ? 1 : pieces.size()) ||
				loaded->spheres.size() != rawSpheres.size())
			{
			std::fprintf(stderr, "AIGeometries loader mismatch id=%d\n", id);
			return 4;
			}
			Add(static_cast<std::uint32_t>(id));
			Add(static_cast<std::uint32_t>(rawPoints.size()));
			for (const auto &point : rawPoints) AddVec(point);
			AddTriangles(rawTriangles);
			Add(static_cast<std::uint32_t>(rawSpheres.size()));
			for (const auto &sphere : rawSpheres)
			{ AddVec(sphere.ptCenter); AddFloat(sphere.fRadius); AddFloat(sphere.fMass); }
			std::map<int, const SStoredPieceProbe *> ordered;
			for (const auto &piece : pieces) ordered.emplace(piece.first, &piece.second);
			Add(static_cast<std::uint32_t>(ordered.size()));
			for (const auto &piece : ordered)
			{
				Add(static_cast<std::uint32_t>(piece.first));
				for (const auto &point : piece.second->verts) AddVec(point);
				AddTriangles(piece.second->tris);
				Add(static_cast<std::uint32_t>(piece.second->weights.size()));
				for (const auto &weight : piece.second->weights)
				{ AddFloat(weight.fWeight); Add(weight.nVertex); Add(weight.nBone); }
				AddFloat(piece.second->fVolume);
				Add(static_cast<std::uint32_t>(piece.second->juncs.size()));
				for (const auto &junction : piece.second->juncs)
				{ AddVec(junction.pt); Add(junction.bGround ? 1 : 0); }
				AddPrecalc(piece.second->precalc, &gridCells);
				points += piece.second->verts.size();
				triangles += piece.second->tris.size();
				precalcTotal += piece.second->precalc.size();
			}
			AddPrecalc(precalc, &gridCells);
			points += rawPoints.size(); triangles += rawTriangles.size();
			spheres += rawSpheres.size(); piecesTotal += pieces.size();
			precalcTotal += precalc.size(); ++geometries;
		}
		catch (const std::exception &error)
		{
			std::fprintf(stderr, "AIGeometries decode failed id=%d phase=%s error=%s\n",
				id, phase, error.what());
			return 5;
		}
		catch (...)
		{
			std::fprintf(stderr, "AIGeometries decode failed id=%d phase=%s\n", id, phase);
			return 5;
		}
	}
	NGScene::CloseAllResources();
	std::printf("available=%zu database=%zu geometries=%zu loose_files=%zu overrides=%zu loose_only=%zu points=%zu triangles=%zu spheres=%zu pieces=%zu precalc=%zu grid_cells=%zu digest=%016llX\n",
		available.size(), ids.size(), geometries, looseIDs.size(), overrides,
		looseOnly, points, triangles, spheres, piecesTotal, precalcTotal, gridCells,
		static_cast<unsigned long long>(digest));
	return available.size() == 2043 && geometries == 1982 &&
		looseIDs.size() == 1974 && overrides == 1974 && looseOnly == 0 &&
		points == 124110 && triangles == 206210 && spheres == 4853 &&
		piecesTotal == 3972 && precalcTotal == 14932 &&
		gridCells == 4224664 && digest == UINT64_C(0xE3F1B8241671D9EC) ? 0 : 6;
}
