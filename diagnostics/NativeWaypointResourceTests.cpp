#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/aiPosition.h"
#include "../Main/aiWaypoint.h"
#include "../FileIO/PortablePackageIndex.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>

static std::uint64_t digest = 14695981039346656037ULL;
static void Add( std::uint32_t value )
{
	for ( int i = 0; i < 4; ++i )
	{
		digest ^= static_cast<unsigned char>(value >> (i * 8));
		digest *= 1099511628211ULL;
	}
}
static void AddFloat( float value )
{
	std::uint32_t bits;
	std::memcpy(&bits, &value, sizeof(bits));
	Add(bits);
}

int main( int argc, char **argv )
{
	if ( argc != 2 ) return 2;
	S2FileIO::PortablePackageIndex index;
	if ( !index.Open(argv[1]) ) return 3;
	NGScene::AddResourceDir(std::filesystem::path(argv[1]).parent_path().string().c_str());
	unsigned int commands = 0;
	unsigned int decodeFailures = 0;
	for ( const auto &entry : index.Entries() )
	{
		try
		{
			NGScene::CResourceOpener resource("Waypoints", entry.first);
			CObj<NAI::CWaypoint> strict = new NAI::CWaypoint;
			strict->operator&( *resource.operator->() );
		}
		catch (...)
		{
			if ( decodeFailures < 10 )
				std::fprintf(stderr, "waypoint decode failed id=%d bytes=%u\n",
					entry.first, entry.second.length);
			if ( entry.first != 8 && entry.first != 9 )
				return 8;
			++decodeFailures;
			continue;
		}
		CObj<NAI::CWaypointLoader> loader = new NAI::CWaypointLoader;
		loader->SetKey(entry.first);
		NAI::CWaypoint *point = loader->GetValue();
		if ( !point ) return 4;
		Add(static_cast<std::uint32_t>(entry.first));
		AddFloat(point->ptPos.x);
		AddFloat(point->ptPos.y);
		AddFloat(point->ptPos.z);
		Add(static_cast<std::uint32_t>(point->nFloor));
		Add(static_cast<std::uint32_t>(point->fRotation));
		Add(static_cast<std::uint32_t>(point->commands.size()));
		commands += static_cast<unsigned int>(point->commands.size());
		for ( const auto &command : point->commands )
		{
			Add(static_cast<std::uint32_t>(command.cmd));
			AddFloat(command.ptPos.x);
			AddFloat(command.ptPos.y);
			AddFloat(command.ptPos.z);
			Add(static_cast<std::uint32_t>(command.pose));
			Add(static_cast<std::uint32_t>(command.dir));
		}
	}
	if ( decodeFailures != 2 )
		return 9;
	NGScene::CloseAllResources();
	std::printf("waypoints=%zu decoded=%zu failures=%u commands=%u digest=%016llX\n",
		index.Entries().size(), index.Entries().size() - decodeFailures,
		decodeFailures, commands, static_cast<unsigned long long>(digest));
	return 0;
}
