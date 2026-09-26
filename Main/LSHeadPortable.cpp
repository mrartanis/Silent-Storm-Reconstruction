#if !defined(_WIN32)
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#include "../DBFormat/DataFormat.h"
#include "LSHead.h"
#include "HeadResourceData.h"
#include "../Misc/BasicShare.h"

namespace NLSHead {

// The native LifeStudio implementation needs no x86 DLL initialization.
// Keep the same shared resource IDs and lazy loaders as the Windows game.
CBasicShare<int, CHeadMeshLoader> shareHeads(135);
static CBasicShare<int, CHeadSequenceLoader> shareSequences(137);
static CLSPtr<LifeStudioHeadAPI::IMMTree> pLSTree;

static void LoadLSTree() {
  if (pLSTree) return;
  pLSTree = LifeStudioHeadAPI::IMMTree::Create();
  if (pLSTree && !pLSTree->Load("tree.mma")) pLSTree = nullptr;
}

void CHeadMeshLoader::Recalc() {
  try {
    SHeadResourceData data;
    LoadHeadResourceData(GetKey(), &data);
    pValue = new CHeadMeshInfo;
    pValue->nVertices.swap(data.nVertices);
    pValue->copys.swap(data.copys);
    pValue->UVs.swap(data.UVs);
    pValue->indices.swap(data.indices);
    pValue->tris.swap(data.tris);
    pValue->pLSAnimators.resize(data.streams.size());
    for (std::size_t i = 0; i < data.streams.size(); ++i) {
      LifeStudioHeadAPI::IAnimator* animator = LifeStudioHeadAPI::IAnimator::Create();
      if (!animator || !animator->Load(
          reinterpret_cast<const char*>(data.streams[i].GetBuffer()),
          data.streams[i].GetSize())) {
        if (animator) animator->Destroy();
        throw std::runtime_error("head animator stream failed to load");
      }
      pValue->pLSAnimators[i] = animator;
      LoadLSTree();
      if (pLSTree) animator->RegisterMacroMuscle(pLSTree->RootMacroMuscle());
    }
  } catch (...) {
    pValue = nullptr;
  }
}

void CHeadSequenceLoader::Recalc() {
  try {
    NGScene::CResourceOpener file("Sequences", GetKey());
    CMemoryStream stream;
    file->Add(1, &stream);
    pValue = new CHeadSequenceInfo;
    LifeStudioHeadAPI::ISequencer* sequencer = LifeStudioHeadAPI::ISequencer::Create();
    if (!sequencer || !sequencer->Load(
        reinterpret_cast<const char*>(stream.GetBuffer()), stream.GetSize())) {
      if (sequencer) sequencer->Destroy();
      throw std::runtime_error("head sequence stream failed to load");
    }
    pValue->pLSSequence = sequencer;
    LoadLSTree();
    if (pLSTree) sequencer->RegisterMMTree(pLSTree);
  } catch (...) {
    pValue = nullptr;
  }
}

CHeadInfo::CHeadInfo(NDb::CComplexHead* complexHead) : pHead(complexHead) {
  if (IsValid(pHead) && IsValid(pHead->pHead)) {
    NDb::CHead* dbHead = pHead->pHead;
    pMaterial = dbHead->pMaterial;
    pMesh = shareHeads.Get(dbHead->GetRecordID());
  }
  if (IsValid(pHead)) {
    pHair = pHead->pHair;
    for (int i = 0; i < 4; ++i) {
      pMeshes[i] = pHead->pMeshes[i];
      pIFMeshes[i] = pHead->pIFMeshes[i];
    }
  }
}

}  // namespace NLSHead

using namespace NLSHead;
REGISTER_SAVELOAD_CLASS(0x11042141, CHeadMeshLoader)
REGISTER_SAVELOAD_CLASS(0x11042142, CHeadSequenceLoader)
REGISTER_SAVELOAD_CLASS(0xA2543120, CHeadInfo)
#endif
