#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GResource.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../third_party/lifestudio/src/NativeHeadData.h"

#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

static std::uint64_t digest = UINT64_C(14695981039346656037);
static void Add( std::uint32_t value )
{
	for ( int n = 0; n < 4; ++n )
	{
		digest ^= static_cast<std::uint8_t>( value >> (8 * n) );
		digest *= UINT64_C(1099511628211);
	}
}
static void AddFloat( float value )
{
	std::uint32_t bits;
	std::memcpy(&bits, &value, sizeof(bits));
	Add(bits);
}
static void AddString( const std::string &value )
{
	Add(static_cast<std::uint32_t>(value.size()));
	for ( unsigned char byte : value ) Add(byte);
}
int main( int argc, char **argv )
{
	if ( argc != 2 ) return 2;
	S2FileIO::PortablePackageIndex package;
	if ( !package.Open(argv[1]) ) return 3;
	NGScene::AddResourceDir(std::filesystem::path(argv[1]).parent_path().string().c_str());
	std::size_t streamsTotal = 0, verticesTotal = 0, musclesTotal = 0;
	for ( const auto &entry : package.Entries() )
	{
		int stage = 0;
		try
		{
			NGScene::CResourceOpener resource("Heads", entry.first);
			stage = 1;
			std::vector<CMemoryStream> streams;
			std::vector<int> nVertices;
			std::vector<CTPoint<int>> copys;
			std::vector<CVec2> uvs;
			std::vector<std::uint16_t> indices;
			std::vector<int> tris;
			resource->Add(1, &streams);
			stage = 2;
			resource->Add(2, &nVertices);
			stage = 3;
			resource->Add(3, &copys);
			stage = 4;
			resource->Add(4, &uvs);
			stage = 5;
			resource->Add(5, &indices);
			stage = 6;
			resource->Add(6, &tris);
			stage = 7;
			if ( streams.empty() || nVertices.empty() || uvs.empty() ||
				indices.empty() || tris.empty() ) return 4;
			Add(static_cast<std::uint32_t>(entry.first));
			Add(static_cast<std::uint32_t>(streams.size()));
			Add(static_cast<std::uint32_t>(uvs.size()));
			Add(static_cast<std::uint32_t>(indices.size()));
			Add(static_cast<std::uint32_t>(tris.size()));
			for ( int count : nVertices ) Add(static_cast<std::uint32_t>(count));
			for ( const auto &copy : copys )
			{
				Add(static_cast<std::uint32_t>(copy.x));
				Add(static_cast<std::uint32_t>(copy.y));
			}
			for ( const auto &uv : uvs )
			{
				AddFloat(uv.x);
				AddFloat(uv.y);
			}
			for ( std::uint16_t index : indices ) Add(index);
			for ( int triangle : tris ) Add(static_cast<std::uint32_t>(triangle));
			for ( CMemoryStream &stream : streams )
			{
				NativeLifeStudio::HeadData head;
				if ( !NativeLifeStudio::DecodeHeadVertices(stream.GetBuffer(), stream.GetSize(), &head) )
				{
					std::fprintf(stderr, "head decode failed id=%d bytes=%u\n",
						entry.first, stream.GetSize());
					return 5;
				}
				++streamsTotal;
				verticesTotal += head.vertexCount;
				musclesTotal += head.muscleCount;
				Add(head.vertexCount);
				Add(head.muscleCount);
				Add(head.boneCount);
				for ( const auto &muscle : head.muscles )
				{
					Add(muscle.type);
					AddString(muscle.name);
					for ( float value : muscle.pointA ) AddFloat(value);
					for ( float value : muscle.pointB ) AddFloat(value);
					for ( float value : muscle.falloffX ) AddFloat(value);
					for ( float value : muscle.falloffY ) AddFloat(value);
				}
				for ( const auto &vertex : head.vertices )
				{
					Add(vertex.index);
					for ( float value : vertex.sourcePosition ) AddFloat(value);
					Add(static_cast<std::uint32_t>(vertex.influences.size()));
					for ( const auto &influence : vertex.influences )
					{
						Add(influence.muscleIndex);
						AddFloat(influence.componentA);
						if ( !std::isfinite(influence.componentB) ||
							influence.componentB < 0.0f || influence.componentB > 1.0f ) return 8;
						// Reconstructed falloff uses host FP math (a few ULPs differ on ARM64).
						Add(static_cast<std::uint32_t>(std::lround(influence.componentB * 1000.0)));
					}
				}
				for ( const auto &vertex : head.implicitVertices )
				{
					Add(vertex.index);
					Add(vertex.type);
					Add(vertex.attribute);
					for ( float value : vertex.sourcePosition ) AddFloat(value);
				}
				for ( const auto &bone : head.bones )
				{
					AddString(bone.name);
					for ( float value : bone.matrixA ) AddFloat(value);
					for ( float value : bone.matrixB ) AddFloat(value);
					for ( std::uint32_t index : bone.muscleIndices ) Add(index);
				}
			}
		}
		catch (...)
		{
			std::fprintf(stderr, "head resource failed id=%d bytes=%u stage=%d\n",
				entry.first, entry.second.length, stage);
			return 6;
		}
	}
	NGScene::CloseAllResources();
	std::printf("heads=%zu streams=%zu vertices=%zu muscles=%zu digest=%016llX\n",
		package.Entries().size(), streamsTotal, verticesTotal, musclesTotal,
		static_cast<unsigned long long>(digest));
	return package.Entries().size() == 134 && streamsTotal == 136 &&
		verticesTotal == 56462 && musclesTotal == 4825 &&
		digest == UINT64_C(0x8A6A6F3A612C18D0) ? 0 : 7;
}
