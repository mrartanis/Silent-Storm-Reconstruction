#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GResource.h"
#include "../Main/HeadResourceData.h"
#if defined(_WIN32)
#include "../DBFormat/DataFormat.h"
#include "../Main/LSHead.h"
#endif
#include "../FileIO/PortablePackageIndex.h"
#include "../third_party/lifestudio/src/NativeHeadData.h"
#include "../third_party/lifestudio/include/LifeStudioHeadAPI.h"

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
	std::uint64_t neutralDigest = UINT64_C(14695981039346656037);
	std::uint64_t neutralCoarseDigest = UINT64_C(14695981039346656037);
	double neutralAbsoluteSum = 0.0;
	for ( const auto &entry : package.Entries() )
	{
		try
		{
			NLSHead::SHeadResourceData data;
			NLSHead::LoadHeadResourceData(entry.first, &data);
			auto &streams = data.streams;
			const auto &nVertices = data.nVertices;
			const auto &copys = data.copys;
			const auto &uvs = data.UVs;
			const auto &indices = data.indices;
			const auto &tris = data.tris;
			if ( streams.empty() || nVertices.empty() || uvs.empty() ||
				indices.empty() || tris.empty() ) return 4;
#if defined(_WIN32)
			if ( entry.first == 11 )
			{
				CObj<NLSHead::CHeadMeshLoader> liveLoader = new NLSHead::CHeadMeshLoader;
				liveLoader->SetKey(entry.first);
				NLSHead::CHeadMeshInfo *live = liveLoader->GetValue();
				if ( !live || live->pLSAnimators.size() != streams.size() ||
					live->nVertices != nVertices || live->copys != copys ||
					live->UVs != uvs || live->indices != indices || live->tris != tris )
					return 9;
			}
#endif
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
				if ( entry.first == 11 && &stream == &streams.front() )
				{
				LifeStudioHeadAPI::IAnimator *animator = LifeStudioHeadAPI::IAnimator::Create();
				if ( !animator ) return 10;
				const bool loaded = animator->Load(
					reinterpret_cast<const char *>(stream.GetBuffer()), stream.GetSize());
				const bool countsMatch = loaded && animator->VerticesCount() == head.vertexCount &&
					animator->MusclesCount() == head.muscleCount &&
					animator->BonesCount() == head.boneCount;
				std::vector<float> neutral(head.vertexCount * 3, 0.0f);
				const bool processed = countsMatch && animator->Process(neutral.data(), 3);
				std::vector<char> saved(stream.GetSize());
				const bool roundTrip = processed && animator->SaveBufferSize() == stream.GetSize() &&
					animator->Save(saved.data()) &&
					std::memcmp(saved.data(), stream.GetBuffer(), saved.size()) == 0;
				animator->Destroy();
				if ( !roundTrip ) return 11;
				for ( float value : neutral )
				{
					if ( !std::isfinite(value) || std::fabs(value) > 10000.0f ) return 12;
					neutralAbsoluteSum += std::fabs(value);
					const std::int32_t quantized = static_cast<std::int32_t>(std::lround(value * 10000.0f));
					const std::int32_t coarse = static_cast<std::int32_t>(std::lround(value * 100.0f));
					for ( int byte = 0; byte < 4; ++byte )
					{
						neutralDigest ^= static_cast<std::uint8_t>(quantized >> (8 * byte));
						neutralDigest *= UINT64_C(1099511628211);
						neutralCoarseDigest ^= static_cast<std::uint8_t>(coarse >> (8 * byte));
						neutralCoarseDigest *= UINT64_C(1099511628211);
					}
				}
			}
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
			std::fprintf(stderr, "head resource failed id=%d bytes=%u\n",
				entry.first, entry.second.length);
			return 6;
		}
	}
	NGScene::CloseAllResources();
	std::printf("heads=%zu streams=%zu vertices=%zu muscles=%zu digest=%016llX neutral=%016llX coarse=%016llX sum_abs=%.6f\n",
		package.Entries().size(), streamsTotal, verticesTotal, musclesTotal,
		static_cast<unsigned long long>(digest),
		static_cast<unsigned long long>(neutralDigest),
		static_cast<unsigned long long>(neutralCoarseDigest), neutralAbsoluteSum);
	return package.Entries().size() == 134 && streamsTotal == 136 &&
		verticesTotal == 56462 && musclesTotal == 4825 &&
		digest == UINT64_C(0x8A6A6F3A612C18D0) ? 0 : 7;
}
