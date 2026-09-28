#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GGeometry.h"
#include "../Main/GObjectInfo.h"
#include "../Main/PortableMeshCodecs.h"
#include "../Main/GObjectInfoLoadCore.h"
#include "../FileIO/PortableGameDatabase.h"
#include "../FileIO/PortablePackageIndex.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <thread>
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
static int Column(const std::vector<std::string> &names, const char *name)
{
	auto it = std::find(names.begin(), names.end(), name);
	return it == names.end() ? -1 : static_cast<int>(it - names.begin());
}
static std::uint32_t AddGeometry(NGScene::CObjectInfo *object)
{
	if (!object) return 0;
	std::vector<STriangle> triangles;
	object->GetPosTriangles(&triangles);
	const auto &positions = object->GetPositions();
	std::vector<std::array<int, 9>> canonical;
	canonical.reserve(triangles.size());
	for (const auto &triangle : triangles)
	{
		std::array<std::array<int, 3>, 3> points;
		const WORD indices[3] = {triangle.i1, triangle.i2, triangle.i3};
		for (int i = 0; i < 3; ++i)
		{
			if (indices[i] >= positions.size()) return 0;
			const CVec3 &point = positions[indices[i]];
			points[i] = {Float2Int(point.x * 4096),
				Float2Int(point.y * 4096), Float2Int(point.z * 4096)};
		}
		std::sort(points.begin(), points.end());
		canonical.push_back({points[0][0], points[0][1], points[0][2],
			points[1][0], points[1][1], points[1][2],
			points[2][0], points[2][1], points[2][2]});
	}
	std::sort(canonical.begin(), canonical.end());
	Add(static_cast<std::uint32_t>(canonical.size()));
	for (const auto &triangle : canonical)
		for (int coordinate : triangle) Add(static_cast<std::uint32_t>(coordinate));
	return static_cast<std::uint32_t>(canonical.size());
}
template <class T>
static NGScene::CObjectInfo *Acquire(CDGPtr<T> &clipper)
{
	for (int attempt = 0; attempt < 20000; ++attempt)
	{
		MarkNewDGFrame();
		clipper.Refresh();
		NGScene::CObjectInfo *object = clipper->GetValue();
		if (object) return object;
		std::this_thread::yield();
	}
	return nullptr;
}
int main(int argc, char **argv)
{
	if (argc != 3) return 2;
	S2FileIO::PortableGameDatabase database;
	std::string error;
	if (!S2FileIO::LoadPortableGameDatabase(argv[1], &database, &error)) return 3;
	std::unordered_set<int> geometryIDs;
	for (const auto &table : database.tables)
		if (table.tableId == 47)
		{
			const int first = Column(table.intNames, "FirstGeometryID");
			const int second = Column(table.intNames, "SecondGeometryID");
			if (first < 0 || second < 0) return 4;
			for (const auto &row : table.intRows)
			{
				if (row[first] > 0) geometryIDs.insert(row[first]);
				if (row[second] > 0) geometryIDs.insert(row[second]);
			}
		}
	const std::filesystem::path resourceDir(argv[2]);
	S2FileIO::PortablePackageIndex index;
	if (!index.Open((resourceDir / "Geometries.res").string())) return 5;
	NGScene::AddResourceDir(resourceDir.string().c_str());
	NGScene::RunResourceLoadingThread();
	int selectedID = -1, selectedPart = -1;
	for (const auto &entry : index.Entries())
	{
		const int id = entry.first;
		if (!geometryIDs.count(id & 0xffff)) continue;
		NGScene::CResourceOpener file("Geometries", id);
		CPtr<NGScene::CObjectInfoPieces> piece = new NGScene::CObjectInfoPieces;
		NGScene::ReadObjectInfoPieces(file.operator->(), piece);
		std::vector<int> partIDs;
		for (const auto &part : piece->faces) partIDs.push_back(part.first);
		std::sort(partIDs.begin(), partIDs.end());
		for (int partID : partIDs)
		{
			const auto &face = piece->faces.at(partID);
			if (face.points.size() >= 4 && face.points.size() < 512 &&
				face.polys.GetTrianglesCount() > 0)
			{
				selectedID = id;
				selectedPart = partID;
				break;
			}
		}
		if (selectedID >= 0) break;
	}
	if (selectedID < 0) return 6;
	NGScene::SClipShare key;
	key.src = NGScene::SPartKey(selectedID & 0xffff, selectedID >> 16);
	key.dwParts = NBuilding::UNBROKEN_BLOCK32;
	key.nSubBlockID = selectedPart;
	key.nClip = 0;
	CDGPtr<NGScene::CSolidObjectInfoClipper> solid = new NGScene::CSolidObjectInfoClipper;
	solid->SetKey(key);
	NGScene::CObjectInfo *solidObject = Acquire(solid);
	const auto solidTriangles = AddGeometry(solidObject);
	CDGPtr<NGScene::CWallObjectInfoClipper> wall = new NGScene::CWallObjectInfoClipper;
	wall->SetKey(key);
	NGScene::CObjectInfo *wallObject = Acquire(wall);
	const auto wallTriangles = AddGeometry(wallObject);
	key.nClip = (0x0201 << 16) | (7 << 8);
	CDGPtr<NGScene::CWallObjectInfoClipper> clipped = new NGScene::CWallObjectInfoClipper;
	clipped->SetKey(key);
	NGScene::CObjectInfo *clippedObject = Acquire(clipped);
	const auto clippedTriangles = AddGeometry(clippedObject);
	NGScene::CloseAllResources();
	std::printf("resource_id=%d part_id=%d solid_triangles=%u wall_triangles=%u clipped_triangles=%u digest=%016llX\n",
		selectedID, selectedPart, solidTriangles, wallTriangles,
		clippedTriangles, static_cast<unsigned long long>(digest));
	return selectedID == 345 && selectedPart == 0 &&
		solidTriangles == 26 && wallTriangles == 10 && clippedTriangles == 8 &&
		digest == UINT64_C(0x99B3282EA22ED8EA) ? 0 : 7;
}
