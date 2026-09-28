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
int main(int argc, char **argv)
{
	if (argc != 3) return 2;
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
