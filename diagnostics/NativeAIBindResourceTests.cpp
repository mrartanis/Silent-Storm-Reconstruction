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
static bool SameMatrix(const SHMatrix &a, const SHMatrix &b)
{
	const float *left[] = { &a._11, &a._12, &a._13, &a._14,
		&a._21, &a._22, &a._23, &a._24, &a._31, &a._32, &a._33,
		&a._34, &a._41, &a._42, &a._43, &a._44 };
	const float *right[] = { &b._11, &b._12, &b._13, &b._14,
		&b._21, &b._22, &b._23, &b._24, &b._31, &b._32, &b._33,
		&b._34, &b._41, &b._42, &b._43, &b._44 };
	for (int i = 0; i < 16; ++i)
		if (std::memcmp(left[i], right[i], sizeof(float)) != 0) return false;
	return true;
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
			for (std::size_t i = 0; i < invBindPoses.size(); ++i)
				if (!SameMatrix(loaded->invBindPoses[i], invBindPoses[i])) return 4;
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
	const auto aiDigest = digest;
	S2FileIO::PortablePackageIndex modelIndex;
	if (!modelIndex.Open((directory / "Binds.res").string())) return 7;
	digest = UINT64_C(14695981039346656037);
	std::size_t modelRecords = 0, modelMatrices = 0;
	for (const auto &entry : modelIndex.Entries())
	{
		const int id = entry.first;
		try
		{
			NGScene::CResourceOpener file("Binds", id);
			vector<SHMatrix> invBindPoses;
			file->Add(4, &invBindPoses);
			CObj<NGScene::CFileBind> loader = new NGScene::CFileBind;
			loader->SetKey(id);
			const auto *loaded = loader->GetValue();
			if (!loaded || loaded->invBindPoses.size() != invBindPoses.size())
			{
				std::fprintf(stderr, "Binds loader mismatch id=%d\n", id);
				return 8;
			}
			for (std::size_t i = 0; i < invBindPoses.size(); ++i)
				if (!SameMatrix(loaded->invBindPoses[i], invBindPoses[i])) return 8;
			Add(static_cast<std::uint32_t>(id));
			Add(static_cast<std::uint32_t>(invBindPoses.size()));
			for (const auto &pose : invBindPoses) AddMatrix(pose);
			modelMatrices += invBindPoses.size(); ++modelRecords;
		}
		catch (const std::exception &error)
		{
			std::fprintf(stderr, "Binds decode failed id=%d error=%s\n", id, error.what());
			return 9;
		}
		catch (...)
		{
			std::fprintf(stderr, "Binds decode failed id=%d\n", id);
			return 9;
		}
	}
	NGScene::CloseAllResources();
	std::printf("ai_records=%zu ai_matrices=%zu ai_digest=%016llX model_records=%zu model_matrices=%zu model_digest=%016llX\n",
		records, matrices, static_cast<unsigned long long>(aiDigest),
		modelRecords, modelMatrices, static_cast<unsigned long long>(digest));
	return records == 211 && matrices == 1577 &&
		aiDigest == UINT64_C(0x5E3D53E8692EC077) &&
		modelRecords == 396 && modelMatrices == 7188 &&
		digest == UINT64_C(0x49A7E7CBF80CB004) ? 0 : 6;
}
