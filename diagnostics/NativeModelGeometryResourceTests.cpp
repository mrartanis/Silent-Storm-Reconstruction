#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GResource.h"
#include "../Main/PortableMeshCodecs.h"
#include "../Main/GObjectInfoLoadCore.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../FileIO/Streams.h"
#include "../DBFormat/DataGeometry.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <exception>

static std::uint64_t digest = UINT64_C(14695981039346656037);
static std::uint64_t objectDigest = UINT64_C(14695981039346656037);
static void AddObject(std::uint32_t value)
{
	for (int byte = 0; byte < 4; ++byte)
	{
		objectDigest ^= static_cast<std::uint8_t>(value >> (byte * 8));
		objectDigest *= UINT64_C(1099511628211);
	}
}
static void AddObjectFloat(float value)
{
	std::uint32_t bits;
	std::memcpy(&bits, &value, sizeof(bits));
	AddObject(bits);
}
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
int main(int argc, char **argv)
{
	if (argc != 3) return 2;
	CFileStream database;
	database.OpenRead(argv[1]);
	NDatabase::Serialize(database, CStructureSaver::READ);
	auto *geometries = NDatabase::GetTable<NDb::CGeometry>();
	if (!geometries) return 3;
	const std::filesystem::path directory(argv[2]);
	S2FileIO::PortablePackageIndex index;
	if (!index.Open((directory / "Geometries.res").string())) return 4;
	NGScene::AddResourceDir(directory.string().c_str());
	std::size_t records = 0, verticesTotal = 0, indicesTotal = 0;
	std::size_t polygonsTotal = 0, weightsTotal = 0, skipped = 0;
	std::size_t invalidIndices = 0, invalidWeights = 0, assembled = 0;
	std::size_t weightedAssembled = 0;
	for (const auto &entry : index.Entries())
	{
		const int id = entry.first;
		const int geometryID = id & 0xffff;
		if (!geometries->GetDBRecord(geometryID))
		{
			++skipped;
			continue;
		}
		try
		{
			NGScene::CResourceOpener file("Geometries", id);
			vector<NGScene::SLoadVertex> vertices;
			vector<WORD> indices, polygons;
			vector<NGScene::SLoadVertexWeight> weights;
			file->Add(1, &vertices);
			file->Add(2, &indices);
			file->Add(3, &polygons);
			file->Add(5, &weights);
			Add(static_cast<std::uint32_t>(id));
			Add(static_cast<std::uint32_t>(vertices.size()));
			for (const auto &vertex : vertices)
			{
				AddVec(vertex.pos); AddVec(vertex.normal);
				AddFloat(vertex.tex.x); AddFloat(vertex.tex.y);
				AddVec(vertex.texU); AddVec(vertex.texV);
				Add(static_cast<std::uint32_t>(vertex.dwColor));
			}
			Add(static_cast<std::uint32_t>(indices.size()));
			for (WORD value : indices)
			{
				Add(value);
				if (value >= vertices.size()) ++invalidIndices;
			}
			Add(static_cast<std::uint32_t>(polygons.size()));
			for (WORD value : polygons) Add(value);
			Add(static_cast<std::uint32_t>(weights.size()));
			for (const auto &weight : weights)
			{
				AddFloat(weight.fWeight);
				Add(static_cast<std::uint32_t>(weight.nVertex));
				Add(static_cast<std::uint32_t>(weight.nBone));
				if (weight.nVertex < 0 ||
					static_cast<std::size_t>(weight.nVertex) >= vertices.size())
					++invalidWeights;
			}
			if (id == 1 || geometryID == 1486 || geometryID == 3087)
			{
				NGScene::SPolygonIndices polygonsData;
				polygonsData.indices = indices;
				polygonsData.polys = polygons;
				CPtr<NGScene::CObjectInfo> object = new NGScene::CObjectInfo;
				NGScene::AssignLoadedObjectInfo(object, vertices, weights, polygonsData);
				AddObject(static_cast<std::uint32_t>(id));
				AddObject(static_cast<std::uint32_t>(object->GetTrisCount()));
				AddObject(static_cast<std::uint32_t>(object->GetPositions().size()));
				AddObject(static_cast<std::uint32_t>(object->GetVertices().size()));
				AddObject(static_cast<std::uint32_t>(object->GetWeights().size()));
				for (const auto &position : object->GetPositions())
				{
					AddObjectFloat(position.x);
					AddObjectFloat(position.y);
					AddObjectFloat(position.z);
				}
				for (WORD positionIndex : object->GetPositionIndices()) AddObject(positionIndex);
				for (const auto &vertex : object->GetVertices())
				{
					AddObject(static_cast<std::uint16_t>(vertex.tex.nU));
					AddObject(static_cast<std::uint16_t>(vertex.tex.nV));
					for (const auto *basis : {&vertex.normal, &vertex.texU, &vertex.texV})
					{
						AddObject(basis->x); AddObject(basis->y);
						AddObject(basis->z); AddObject(basis->w);
					}
				}
				for (const auto &weight : object->GetWeights())
					for (int bone = 0; bone < 4; ++bone)
					{
						AddObjectFloat(weight.fWeights[bone]);
						AddObject(weight.nWeights[bone]);
						AddObject(weight.cBoneIndices[bone]);
					}
				for (WORD index : object->GetGeometry().indices) AddObject(index);
				for (WORD polygon : object->GetGeometry().polys) AddObject(polygon);
				++assembled;
				if (!weights.empty()) ++weightedAssembled;
			}
			++records;
			verticesTotal += vertices.size();
			indicesTotal += indices.size();
			polygonsTotal += polygons.size();
			weightsTotal += weights.size();
		}
		catch (const std::exception &error)
		{
			std::fprintf(stderr, "Geometries decode failed id=%d error=%s\n",
				id, error.what());
			return 5;
		}
		catch (...)
		{
			std::fprintf(stderr, "Geometries decode failed id=%d\n", id);
			return 5;
		}
	}
	NGScene::CloseAllResources();
	std::printf("records=%zu skipped=%zu vertices=%zu indices=%zu polygons=%zu weights=%zu invalid_indices=%zu invalid_weights=%zu digest=%016llX assembled=%zu weighted=%zu object_digest=%016llX\n",
		records, skipped, verticesTotal, indicesTotal, polygonsTotal,
		weightsTotal, invalidIndices, invalidWeights,
		static_cast<unsigned long long>(digest), assembled, weightedAssembled,
		static_cast<unsigned long long>(objectDigest));
	return records == 6824 && skipped == 966 &&
		verticesTotal == 980946 && indicesTotal == 1994631 &&
		polygonsTotal == 620259 && weightsTotal == 361772 &&
		invalidIndices == 0 && invalidWeights == 0 && assembled == 3 &&
		weightedAssembled >= 1 &&
		digest == UINT64_C(0xC7F41DAD5636E94A) &&
		objectDigest == UINT64_C(0xC421973B117E0717) ? 0 : 6;
}
