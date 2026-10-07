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
	NGScene::StopResourceLoadingThread();
	// A packed upper layer must beat a loose lower layer (the shipped patch
	// plus an HD archive), while a loose file within the upper layer wins again.
	const auto upper = std::filesystem::path(argv[2]) / "packed-upper";
	std::filesystem::create_directories(upper);
	std::filesystem::remove(upper / "Waypoints" / std::to_string(id));
	std::filesystem::copy_file(argv[1], upper / "waypoints.res", std::filesystem::copy_options::overwrite_existing);
	NGScene::AddResourceDir(upper.string().c_str());
	if ( ReadResource(id) != expected ) return 10;
	CObj<NGScene::CFileRequest> packedRequest = new NGScene::CFileRequest("waypoints", id);
	packedRequest->Read();
	std::vector<unsigned char> packedBytes(packedRequest->GetStream()->GetSize());
	if (!packedBytes.empty()) packedRequest->GetStream()->Read(packedBytes.data(), static_cast<unsigned>(packedBytes.size()));
	if ( packedBytes != expected ) return 11;
	const int looseOnlyId = -1234567;
	{
		std::ofstream file(fixture / std::to_string(looseOnlyId), std::ios::binary);
		file.write(marker, sizeof(marker)-1);
	}
	if (!NGScene::CResourceFileOpener::DoesExist("waypoints",looseOnlyId) ||
		ReadResource(looseOnlyId) != overridden ||
		NGScene::CResourceFileOpener::DoesExist("waypoints",looseOnlyId-1)) return 12;
	std::filesystem::create_directories(upper / "Waypoints");
	std::filesystem::copy_file(loose,upper / "Waypoints" / std::to_string(id),std::filesystem::copy_options::overwrite_existing);
	if ( ReadResource(id) != overridden ) return 13;
	packedRequest = nullptr;
	NGScene::CloseAllResources();
	NGScene::ClearResourceDirs();
	// HD opt-out affects only the visual overlay, including async requests.
	const auto previousDirectory=std::filesystem::current_path();
	const auto visualRoot=std::filesystem::absolute(std::filesystem::path(argv[2]) / "hd-toggle");
	const auto shardSource=std::filesystem::absolute(argv[1]);
	std::filesystem::create_directories(visualRoot / "res" / "Textures");
	std::filesystem::create_directories(visualRoot / "res-hd" / "Textures");
	const int visualId=7654321;
	std::filesystem::copy_file(shardSource,visualRoot / "res-hd" / "Textures-0001.res",std::filesystem::copy_options::overwrite_existing);
	for(const auto& layer:{"res","res-hd"}) {
		std::ofstream file(visualRoot / layer / "Textures" / std::to_string(visualId),std::ios::binary);
		file << layer;
	}
	std::filesystem::current_path(visualRoot);
	NGScene::AddBaseResourceDirs();
	{
		NGScene::CResourceFileOpener shard("Textures",id);
		std::vector<unsigned char> bytes(shard.GetStream()->GetSize());
		if(!bytes.empty())shard.GetStream()->Read(bytes.data(),static_cast<unsigned>(bytes.size()));
		if(bytes!=expected)return 21;
	}
	auto readVisual=[&]() {
		NGScene::CResourceFileOpener opener("Textures",visualId);
		CDataStream* source=opener.GetStream();
		std::string text(source->GetSize(),'\0');
		if(!text.empty())source->Read(&text[0],static_cast<unsigned>(text.size()));
		return text;
	};
	if(!NGScene::HDTexturesEnabled() || readVisual()!="res-hd")return 14;
	const auto revision=NGScene::GetTextureResourceRevision();
	NGScene::SetHDTexturesEnabled(true);
	if(NGScene::GetTextureResourceRevision()!=revision)return 15;
	const auto networkDirs=NGScene::GetNetworkResourceDirectories();
	if(networkDirs.size()!=1)return 16;
	NGScene::SetHDTexturesEnabled(false);
	if(NGScene::HDTexturesEnabled() || readVisual()!="res" ||
	   NGScene::GetTextureResourceRevision()!=revision+1 || NGScene::GetNetworkResourceDirectories()!=networkDirs)return 17;
	NGScene::RunResourceLoadingThread();
	CObj<NGScene::CFileRequest> visualRequest=new NGScene::CFileRequest("Textures",visualId);
	NGScene::AddFileRequest(visualRequest);
	for(int retry=0;retry<500 && !visualRequest->IsReady();++retry)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	if(!visualRequest->IsReady())return 18;
	char visualBytes[3]{};
	visualRequest->GetStream()->Read(visualBytes,3);
	if(std::string(visualBytes,3)!="res")return 19;
	visualRequest=nullptr;
	NGScene::ReleaseFileRequestHolder();
	NGScene::StopResourceLoadingThread();
	NGScene::SetHDTexturesEnabled(true);
	if(readVisual()!="res-hd" || NGScene::GetTextureResourceRevision()!=revision+2)return 20;
	NGScene::CloseAllResources();
	NGScene::ClearResourceDirs();
	std::filesystem::current_path(previousDirectory);
	std::printf("package_id=%d package_bytes=%zu loose_bytes=%zu async_bytes=%zu\n",
		id, expected.size(), overridden.size(), asyncBytes.size());
	return 0;
}
