#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GBind.h"
#include "../Main/GAnimFormat.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../FileIO/Streams.h"
#include "../DBFormat/DataFormat.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <array>
#include <vector>
#include <algorithm>

class CProbeSkeletonPose : public CFuncBase<NAnimation::SSkeletonPose>
{
	OBJECT_BASIC_METHODS(CProbeSkeletonPose);
public:
	void Set(const NAnimation::SSkeletonPose &pose)
	{ value = pose; Updated(); }
};
class CRecordIterator : public CDBIteratorBase
{
public:
	explicit CRecordIterator(const CDBTableBase &table) : CDBIteratorBase(table) {}
	CDBRecord *Get() const { return CDBIteratorBase::Get(); }
};

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
static std::array<float, 16> MatrixFields(const SHMatrix &matrix)
{
	return {{
		matrix._11, matrix._12, matrix._13, matrix._14,
		matrix._21, matrix._22, matrix._23, matrix._24,
		matrix._31, matrix._32, matrix._33, matrix._34,
		matrix._41, matrix._42, matrix._43, matrix._44
	}};
}
static void AddMatrix(const SHMatrix &matrix)
{
	for (float field : MatrixFields(matrix)) AddFloat(field);
}
static void AddMatrices(const NGScene::SSkeletonMatrices &matrices)
{
	Add(static_cast<std::uint32_t>(matrices.size()));
	for (const auto &matrix : matrices) AddMatrix(matrix);
}
struct SCandidate
{
	int bindID = -1;
	int skeletonID = -1;
	int bones = 0;
	int modelID = 0;
};
static bool TestCandidate(const SCandidate &candidate, bool modelBind = false)
{
	CObj<NGScene::CFileAIBind> aiLoader;
	CObj<NGScene::CFileBind> modelLoader;
	CPtrFuncBase<NGScene::CFileBindInfo> *bindLoader = nullptr;
	if (modelBind)
	{
		modelLoader = new NGScene::CFileBind;
		modelLoader->SetKey(candidate.bindID);
		bindLoader = modelLoader;
	}
	else
	{
		aiLoader = new NGScene::CFileAIBind;
		aiLoader->SetKey(candidate.bindID);
		bindLoader = aiLoader;
	}
	CObj<NAnimation::CFileSkeleton> skeletonLoader = new NAnimation::CFileSkeleton;
	skeletonLoader->SetKey(candidate.skeletonID);
	CDGPtr<CPtrFuncBase<NGScene::CFileBindInfo>> binds(bindLoader);
	CDGPtr<CPtrFuncBase<NAnimation::CFileSkeletonInfo>> skeleton(skeletonLoader);
	binds.Refresh(); skeleton.Refresh();
	const auto *bindInfo = binds->GetValue();
	const auto *skeletonInfo = skeleton->GetValue();
	if (!bindInfo || !skeletonInfo ||
		bindInfo->invBindPoses.size() != static_cast<std::size_t>(candidate.bones) ||
		skeletonInfo->bones.size() != static_cast<std::size_t>(candidate.bones))
		return false;
	NAnimation::SSkeletonPose pose;
	for (const auto &bone : skeletonInfo->bones)
	{
		NAnimation::SBonePose value;
		value.nParent = bone.nParent;
		value.pos = bone.pos; value.rot = bone.rot; value.scale = bone.scale;
		if (value.nParent >= static_cast<int>(pose.size())) return false;
		pose.push_back(value);
	}
	CObj<CProbeSkeletonPose> source = new CProbeSkeletonPose;
	source->Set(pose);
	CObj<NGScene::CBind> bind = new NGScene::CBind;
	bind->pAnimation = source;
	bind->pBinds = bindLoader;
	bind->pSkeleton = skeletonLoader;
	CDGPtr<CFuncBase<NGScene::SSkeletonMatrices>> result(bind);
	result.Refresh();
	const auto first = result->GetValue();
	if (first.size() != pose.size()) return false;
	if (modelBind) Add(candidate.modelID);
	Add(candidate.bindID); Add(candidate.skeletonID);
	Add(skeletonInfo->bScale ? 1 : 0);
	AddMatrices(first);
	// A second DG frame must invalidate CBind via the source pose and recompute
	// global parent transforms. This is core animation/collision data, no GPU.
	pose[0].pos.x += 0.25f;
	pose[1].pos.y += 0.5f;
	source->Set(pose);
	MarkNewDGFrame();
	if (!result.Refresh()) return false;
	const auto second = result->GetValue();
	if (second.size() != first.size()) return false;
	AddMatrices(second);
	bool changed = false;
	for (std::size_t i = 0; i < first.size(); ++i)
		if (first[i]._14 != second[i]._14 ||
			first[i]._24 != second[i]._24 || first[i]._34 != second[i]._34)
			changed = true;
	if (!changed) return false;
	// Only the bScale branch uses the animated pose's scale. In the other
	// branch the bind uses static scale from the skeleton resource.
	pose[0].scale.x += 0.125f;
	source->Set(pose);
	MarkNewDGFrame();
	if (!result.Refresh()) return false;
	const auto third = result->GetValue();
	if (third.size() != second.size()) return false;
	AddMatrices(third);
	bool scaleChanged = false;
	for (std::size_t i = 0; i < second.size(); ++i)
		if (MatrixFields(second[i]) != MatrixFields(third[i]))
			scaleChanged = true;
	return scaleChanged == skeletonInfo->bScale;
}
int main(int argc, char **argv)
{
	if (argc != 3) return 2;
	const std::filesystem::path directory(argv[1]);
	S2FileIO::PortablePackageIndex binds, skeletons, modelBinds;
	if (!binds.Open((directory / "AIBinds.res").string()) ||
		!skeletons.Open((directory / "Skeletons.res").string()) ||
		!modelBinds.Open((directory / "Binds.res").string())) return 3;
	NGScene::AddResourceDir(directory.string().c_str());
	std::map<int, int> nonScaled, scaled;
	for (const auto &entry : skeletons.Entries())
	{
		CObj<NAnimation::CFileSkeleton> loader = new NAnimation::CFileSkeleton;
		loader->SetKey(entry.first);
		CDGPtr<CPtrFuncBase<NAnimation::CFileSkeletonInfo>> pin(loader);
		pin.Refresh();
		const auto *info = pin->GetValue();
		if (!info || info->bones.size() < 2) continue;
		bool ordered = true;
		for (std::size_t i = 0; i < info->bones.size(); ++i)
			if (info->bones[i].nParent >= static_cast<int>(i)) ordered = false;
		if (!ordered) continue;
		auto &pool = info->bScale ? scaled : nonScaled;
		pool.emplace(static_cast<int>(info->bones.size()), entry.first);
	}
	SCandidate picks[2];
	for (const auto &entry : binds.Entries())
	{
		CObj<NGScene::CFileAIBind> loader = new NGScene::CFileAIBind;
		loader->SetKey(entry.first);
		CDGPtr<CPtrFuncBase<NGScene::CFileBindInfo>> pin(loader);
		pin.Refresh();
		const auto *info = pin->GetValue();
		if (!info) continue;
		const int count = static_cast<int>(info->invBindPoses.size());
		const auto plain = nonScaled.find(count);
		const auto withScale = scaled.find(count);
		if (picks[0].bindID < 0 && plain != nonScaled.end())
			picks[0] = {entry.first, plain->second, count};
		if (picks[1].bindID < 0 && withScale != scaled.end())
			picks[1] = {entry.first, withScale->second, count};
		if (picks[0].bindID >= 0 && picks[1].bindID >= 0) break;
	}
	if (picks[0].bindID < 0 || picks[1].bindID < 0) return 4;
	if (!TestCandidate(picks[0]) || !TestCandidate(picks[1])) return 5;
	const auto aiDigest = digest;
	digest = UINT64_C(14695981039346656037);
	CFileStream database;
	database.OpenRead(argv[2]);
	NDatabase::Serialize(database, CStructureSaver::READ);
	auto *models = NDatabase::GetTable(1); // registered "Models"/CRndModel table
	if (!models) return 7;
	std::vector<SCandidate> modelCandidates;
	CRecordIterator modelIt(*models);
	while (modelIt.MoveNext())
	{
		const int modelID = modelIt.Get()->GetRecordID();
		SRand random;
		CObj<NDb::CModel> model = NDb::GetModelVariant(modelID, &random);
		if (!model) continue;
		if (!model->pGeometry.GetPtr() || !model->pSkeleton.GetPtr()) continue;
		const int geometryID = model->pGeometry->GetRecordID();
		const int skeletonID = model->pSkeleton->GetRecordID();
		if (modelBinds.Entries().find(geometryID) == modelBinds.Entries().end() ||
			skeletons.Entries().find(skeletonID) == skeletons.Entries().end()) continue;
		CObj<NGScene::CFileBind> modelLoader = new NGScene::CFileBind;
		modelLoader->SetKey(geometryID);
		CObj<NAnimation::CFileSkeleton> skeletonLoader = new NAnimation::CFileSkeleton;
		skeletonLoader->SetKey(skeletonID);
		const auto *bindInfo = modelLoader->GetValue();
		const auto *skeletonInfo = skeletonLoader->GetValue();
		if (!bindInfo || !skeletonInfo || skeletonInfo->bones.size() < 2 ||
			bindInfo->invBindPoses.size() != skeletonInfo->bones.size()) continue;
		modelCandidates.push_back({geometryID, skeletonID,
			static_cast<int>(skeletonInfo->bones.size()), modelID});
	}
	std::sort(modelCandidates.begin(), modelCandidates.end(),
		[](const SCandidate &a, const SCandidate &b) { return a.modelID < b.modelID; });
	if (modelCandidates.empty()) return 8;
	const SCandidate modelPick = modelCandidates.front();
	if (!TestCandidate(modelPick, true)) return 9;
	NGScene::CloseAllResources();
	std::printf("non_scaled_bind=%d skeleton=%d bones=%d scaled_bind=%d skeleton=%d bones=%d ai_digest=%016llX model_pairs=%zu model=%d bind=%d skeleton=%d bones=%d model_digest=%016llX\n",
		picks[0].bindID, picks[0].skeletonID, picks[0].bones,
		picks[1].bindID, picks[1].skeletonID, picks[1].bones,
		static_cast<unsigned long long>(aiDigest), modelCandidates.size(),
		modelPick.modelID, modelPick.bindID, modelPick.skeletonID, modelPick.bones,
		static_cast<unsigned long long>(digest));
	return picks[0].bindID == 2 && picks[0].skeletonID == 2 &&
		picks[0].bones == 42 && picks[1].bindID == 236 &&
		picks[1].skeletonID == 50 && picks[1].bones == 5 &&
		aiDigest == UINT64_C(0xAD6A5D46B06FF2AB) &&
		modelCandidates.size() == 110 && modelPick.modelID == 1248 &&
		modelPick.bindID == 1486 && modelPick.skeletonID == 142 &&
		modelPick.bones == 9 && digest == UINT64_C(0xEDD325C53B7749A8) ? 0 : 6;
}
