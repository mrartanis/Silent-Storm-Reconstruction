#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GResource.h"
#include "../FileIO/PortablePackageIndex.h"

#include <cstdio>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>

static std::vector<unsigned char> ReadResource( int id )
{
	NGScene::CResourceFileOpener opener( "waypoints", id );
	CDataStream *stream = opener.GetStream();
	std::vector<unsigned char> bytes( stream->GetSize() );
	if ( !bytes.empty() )
		stream->Read( bytes.data(), static_cast<unsigned int>( bytes.size() ) );
	return bytes;
}

int main( int argc, char **argv )
{
	if ( argc != 3 )
		return 2;
	S2FileIO::PortablePackageIndex index;
	if ( !index.Open( argv[1] ) || index.Entries().empty() )
		return 3;
	const int id = index.Entries().begin()->first;
	std::vector<std::uint8_t> expected;
	if ( !index.Read( id, &expected ) )
		return 4;
	std::string baseDir = std::filesystem::path(argv[1]).parent_path().string();
	std::replace(baseDir.begin(), baseDir.end(), '/', '\\');
	NGScene::AddResourceDir(baseDir.c_str());
	if ( !NGScene::CResourceFileOpener::DoesExist("waypoints", id) ||
		ReadResource(id) != expected )
		return 5;

	const std::filesystem::path fixture = std::filesystem::path(argv[2]) / "Waypoints";
	std::filesystem::create_directories(fixture);
	const std::filesystem::path loose = fixture / std::to_string(id);
	const char marker[] = "LOOSE-OVERRIDE";
	{
		std::ofstream file(loose, std::ios::binary | std::ios::trunc);
		file.write(marker, sizeof(marker) - 1);
		if ( !file ) return 6;
	}
	NGScene::AddResourceDir(argv[2]);
	const auto overridden = ReadResource(id);
	if ( overridden != std::vector<unsigned char>(marker, marker + sizeof(marker) - 1) )
		return 7;
	NGScene::RunResourceLoadingThread();
	CObj<NGScene::CFileRequest> request = new NGScene::CFileRequest("waypoints", id);
	NGScene::AddFileRequest(request);
	for ( int retry = 0; retry < 500 && !request->IsReady(); ++retry )
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	if ( !request->IsReady() )
		return 8;
	CMemoryStream *stream = request->GetStream();
	std::vector<unsigned char> asyncBytes(stream->GetSize());
	if ( !asyncBytes.empty() )
		stream->Read(asyncBytes.data(), static_cast<unsigned int>(asyncBytes.size()));
	if ( asyncBytes != overridden )
		return 9;
	request = nullptr;
	NGScene::ReleaseFileRequestHolder();
	NGScene::CloseAllResources();
	std::printf("package_id=%d package_bytes=%zu loose_bytes=%zu async_bytes=%zu\n",
		id, expected.size(), overridden.size(), asyncBytes.size());
	return 0;
}
