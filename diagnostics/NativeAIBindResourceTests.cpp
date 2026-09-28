#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GBind.h"
#include "../FileIO/PortablePackageIndex.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>

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
static void AddMatrix(const SHMatrix &matrix)
{
	const float fields[16] = {
		matrix._11, matrix._12, matrix._13, matrix._14,
		matrix._21, matrix._22, matrix._23, matrix._24,
		matrix._31, matrix._32, matrix._33, matrix._34,
		matrix._41, matrix._42, matrix._43, matrix._44
	};
	for (float field : fields) AddFloat(field);
}
int main(int argc, char **argv)
{
	if (argc != 2) return 2;
	const std::filesystem::path directory(argv[1]);
	S2FileIO::PortablePackageIndex index;
	if (!index.Open((directory / "AIBinds.res").string())) return 3;
	NGScene::AddResourceDir(directory.string().c_str());
	std::size_t records = 0, matrices = 0;
	for (const auto &entry : index.Entries())
	{
		const int id = entry.first;
		try
		{
			NGScene::CResourceOpener file("AIBinds", id);
			vector<SHMatrix> invBindPoses;
			file->Add(4, &invBindPoses);
			CObj<NGScene::CFileAIBind> loader = new NGScene::CFileAIBind;
			loader->SetKey(id);
			const auto *loaded = loader->GetValue();
			if (!loaded || loaded->invBindPoses.size() != invBindPoses.size())
			{
				std::fprintf(stderr, "AIBinds loader mismatch id=%d\n", id);
				return 4;
			}
			Add(static_cast<std::uint32_t>(id));
			Add(static_cast<std::uint32_t>(invBindPoses.size()));
			for (const auto &pose : invBindPoses) AddMatrix(pose);
			matrices += invBindPoses.size(); ++records;
		}
		catch (const std::exception &error)
		{
			std::fprintf(stderr, "AIBinds decode failed id=%d error=%s\n", id, error.what());
			return 5;
		}
		catch (...)
		{
			std::fprintf(stderr, "AIBinds decode failed id=%d\n", id);
			return 5;
		}
	}
	NGScene::CloseAllResources();
	std::printf("records=%zu matrices=%zu digest=%016llX\n", records, matrices,
		static_cast<unsigned long long>(digest));
	return records == 211 && matrices == 1577 &&
		digest == UINT64_C(0x5E3D53E8692EC077) ? 0 : 6;
}
