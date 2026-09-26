#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GResource.h"
#include "../Main/HeadResourceData.h"
#include "../DBFormat/DataFormat.h"
#include "../Main/LSHead.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../third_party/lifestudio/include/LifeStudioHeadAPIMMTS.h"
#include "../third_party/lifestudio/include/LifeStudioHeadAPI.h"
#include "../third_party/lifestudio/src/NativeSequenceData.h"

#include <cstdio>
#include <cmath>
#include <filesystem>

int main(int argc, char** argv) {
  if (argc != 3) return 2;
  S2FileIO::PortablePackageIndex package;
  if (!package.Open(argv[1])) return 3;
  std::filesystem::current_path(std::filesystem::path(argv[2]).parent_path());
  NGScene::AddResourceDir(std::filesystem::path(argv[1]).parent_path().string().c_str());
  LifeStudioHeadAPI::IMMTree* tree = LifeStudioHeadAPI::IMMTree::Create();
  if (!tree || !tree->Load(argv[2]) || !tree->RootMacroMuscle()) return 8;
  NLSHead::SHeadResourceData head;
  NLSHead::LoadHeadResourceData(11, &head);
  if (head.streams.empty()) return 9;
  LifeStudioHeadAPI::IAnimator* animator = LifeStudioHeadAPI::IAnimator::Create();
  if (!animator || !animator->Load(
      reinterpret_cast<const char*>(head.streams.front().GetBuffer()),
      head.streams.front().GetSize())) return 10;
  animator->RegisterMacroMuscle(tree->RootMacroMuscle());
  std::vector<float> neutral(animator->VerticesCount() * 3, 0.0f);
  std::vector<float> expression(neutral.size(), 0.0f);
  if (!animator->Process(neutral.data(), 3)) return 11;
  // Blink, speech, and a held expression are all used by the game.
  const int ids[] = {6005, 371, 7552};
  for (int id : ids) {
    if (package.Entries().find(id) == package.Entries().end()) return 4;
    NGScene::CResourceOpener file("Sequences", id);
    CMemoryStream stream;
    file->Add(1, &stream);
    NativeLifeStudio::SequenceHeader header;
    std::vector<NativeLifeStudio::SequenceTrack> tracks;
    if (!NativeLifeStudio::DecodeSequenceHeader(stream.GetBuffer(), stream.GetSize(), &header) ||
        !NativeLifeStudio::DecodeSequenceTracks(stream.GetBuffer(), stream.GetSize(), &tracks))
      return 5;
    LifeStudioHeadAPI::ISequencer* sequencer = LifeStudioHeadAPI::ISequencer::Create();
    if (!sequencer) return 6;
    const bool loaded = sequencer->Load(
        reinterpret_cast<const char*>(stream.GetBuffer()), stream.GetSize());
    const bool equal = loaded && sequencer->SequenceTime() == static_cast<int>(header.duration) &&
        sequencer->TracksCount() == static_cast<int>(tracks.size());
    CObj<NLSHead::CHeadSequenceLoader> liveLoader = new NLSHead::CHeadSequenceLoader;
    liveLoader->SetKey(id);
    NLSHead::CHeadSequenceInfo* live = liveLoader->GetValue();
    const bool liveEqual = live && live->pLSSequence &&
        live->pLSSequence->SequenceTime() == static_cast<int>(header.duration) &&
        live->pLSSequence->TracksCount() == static_cast<int>(tracks.size());
    if (equal && id == 7552) {
      sequencer->RegisterMMTree(tree);
      sequencer->RenderMacroMuscles(animator, 500);
      animator->ComputePhysics();
      if (!animator->Process(expression.data(), 3)) return 12;
      std::size_t changed = 0;
      for (std::size_t i = 0; i < expression.size(); ++i) {
        if (!std::isfinite(expression[i])) return 13;
        if (std::fabs(expression[i] - neutral[i]) > 0.000001f) ++changed;
      }
      std::printf("expression_changed_coordinates=%zu\n", changed);
      if (changed == 0) return 14;
    }
    sequencer->Destroy();
    if (!equal || !liveEqual) return 7;
    std::printf("sequence=%d duration=%u tracks=%zu\n", id, header.duration, tracks.size());
  }
  animator->Destroy();
  tree->Destroy();
  NGScene::CloseAllResources();
  return 0;
}
