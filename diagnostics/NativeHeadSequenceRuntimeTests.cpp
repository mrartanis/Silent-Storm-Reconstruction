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
#include "../DBFormat/DataAck.h"
#include "../DBFormat/DataFaceGen.h"
#include "../Main/LSHead.h"
#include "../FileIO/PortablePackageIndex.h"
#include "../FileIO/Streams.h"
#include "../third_party/lifestudio/include/LifeStudioHeadAPIMMTS.h"
#include "../third_party/lifestudio/include/LifeStudioHeadAPI.h"
#include "../third_party/lifestudio/src/NativeSequenceData.h"

#include <cstdio>
#include <cmath>
#include <filesystem>
#include <cstdint>
#include <set>
#include <algorithm>
#include <iterator>

static void Add32(std::uint64_t *hash, std::uint32_t value) {
  for (int byte = 0; byte < 4; ++byte) {
    *hash ^= static_cast<std::uint8_t>(value >> (byte * 8));
    *hash *= UINT64_C(1099511628211);
  }
}
static std::uint64_t HashIDs(const std::set<int> &ids) {
  std::uint64_t hash = UINT64_C(14695981039346656037);
  for (int id : ids) Add32(&hash, static_cast<std::uint32_t>(id));
  return hash;
}

static int CheckGameSequenceCorpus(const S2FileIO::PortablePackageIndex &package,
                                   const char *databasePath) {
  CFileStream database;
  database.OpenRead(databasePath);
  NDatabase::Serialize(database, CStructureSaver::READ);
  auto *table = NDatabase::GetTable<NDb::CSequence>();
  if (!table) return 15;
  std::set<int> ids, idleIDs;
  CDBIterator<NDb::CSequence> record(*table);
  while (record.MoveNext()) {
    ids.insert(record.Get()->GetRecordID());
    if (record.Get()->bIdleAnimation) idleIDs.insert(record.Get()->GetRecordID());
  }
  std::set<int> voiceIDs, selectableVoiceIDs, expressionIDs;
  if (auto *acks = NDatabase::GetTable<NDb::CDBAckInfo>()) {
    CDBIterator<NDb::CDBAckInfo> ack(*acks);
    while (ack.MoveNext())
      for (std::size_t slot = 0; slot < ack.Get()->voices.size(); ++slot) {
        const auto &voice = ack.Get()->voices[slot];
        if (!voice.pSequence) continue;
        voiceIDs.insert(voice.pSequence->GetRecordID());
        // CDBAckInfo::GetVoice selects a nondefault slot only when it has sound.
        if (slot == 0 || voice.pSound)
          selectableVoiceIDs.insert(voice.pSequence->GetRecordID());
      }
  }
  if (auto *expressions = NDatabase::GetTable<NDb::CFaceExpression>()) {
    CDBIterator<NDb::CFaceExpression> expression(*expressions);
    while (expression.MoveNext())
      if (expression.Get()->pSequence)
        expressionIDs.insert(expression.Get()->pSequence->GetRecordID());
  }
  std::set<int> selectedIDs = selectableVoiceIDs;
  selectedIDs.insert(expressionIDs.begin(), expressionIDs.end());
  selectedIDs.insert(idleIDs.begin(), idleIDs.end());
  std::size_t present = 0, missing = 0, bytes = 0, tracksTotal = 0;
  std::set<int> presentIDs;
  std::uint64_t digest = UINT64_C(14695981039346656037);
  std::uint64_t missingDigest = UINT64_C(14695981039346656037);
  for (int id : ids) {
    if (!NGScene::CResourceFileOpener::DoesExist("Sequences", id)) {
      ++missing;
      Add32(&missingDigest, static_cast<std::uint32_t>(id));
      continue;
    }
    try {
      NGScene::CResourceOpener file("Sequences", id);
      CMemoryStream stream;
      file->Add(1, &stream);
      NativeLifeStudio::SequenceHeader header;
      std::vector<NativeLifeStudio::SequenceTrack> tracks;
      if (!NativeLifeStudio::DecodeSequenceHeader(stream.GetBuffer(), stream.GetSize(), &header) ||
          !NativeLifeStudio::DecodeSequenceTracks(stream.GetBuffer(), stream.GetSize(), &tracks)) {
        std::fprintf(stderr, "sequence corpus strict decode failed id=%d\n", id);
        return 16;
      }
      CObj<NLSHead::CHeadSequenceLoader> loader = new NLSHead::CHeadSequenceLoader;
      loader->SetKey(id);
      const auto *loaded = loader->GetValue();
      if (!loaded || !loaded->pLSSequence ||
          loaded->pLSSequence->SequenceTime() != static_cast<int>(header.duration) ||
          loaded->pLSSequence->TracksCount() != static_cast<int>(tracks.size())) {
        std::fprintf(stderr, "sequence corpus game loader mismatch id=%d\n", id);
        return 17;
      }
      Add32(&digest, static_cast<std::uint32_t>(id));
      Add32(&digest, static_cast<std::uint32_t>(stream.GetSize()));
      Add32(&digest, header.duration);
      Add32(&digest, static_cast<std::uint32_t>(tracks.size()));
      const auto *raw = static_cast<const std::uint8_t *>(stream.GetBuffer());
      for (int offset = 0; offset < stream.GetSize(); ++offset) {
        digest ^= raw[offset];
        digest *= UINT64_C(1099511628211);
      }
      bytes += stream.GetSize();
      tracksTotal += tracks.size();
      ++present;
      presentIDs.insert(id);
    } catch (...) {
      std::fprintf(stderr, "sequence corpus resource failed id=%d\n", id);
      return 18;
    }
  }
  std::set<int> packageWithoutDB;
  for (const auto *family : {&voiceIDs, &selectableVoiceIDs, &expressionIDs, &idleIDs, &selectedIDs}) {
    std::set<int> absent;
    std::set_difference(family->begin(), family->end(),
      presentIDs.begin(), presentIDs.end(), std::inserter(absent, absent.end()));
    std::uint64_t familyDigest = UINT64_C(14695981039346656037);
    std::uint64_t absentDigest = UINT64_C(14695981039346656037);
    for (int id : *family) Add32(&familyDigest, static_cast<std::uint32_t>(id));
    for (int id : absent) Add32(&absentDigest, static_cast<std::uint32_t>(id));
    const char *name = family == &voiceIDs ? "voice_raw" :
      family == &selectableVoiceIDs ? "voice_selectable" :
      family == &expressionIDs ? "expression" :
      family == &idleIDs ? "idle" : "union";
    std::printf("sequence_refs family=%s ids=%zu missing=%zu digest=%016llX missing_digest=%016llX\n",
      name, family->size(), absent.size(),
      static_cast<unsigned long long>(familyDigest),
      static_cast<unsigned long long>(absentDigest));
    if (!absent.empty())
      std::printf("sequence_refs_missing_first family=%s id=%d\n", name, *absent.begin());
  }
  std::set<int> selectedMissing;
  std::set_difference(selectedIDs.begin(), selectedIDs.end(),
    presentIDs.begin(), presentIDs.end(),
    std::inserter(selectedMissing, selectedMissing.end()));
  for (int id : selectedMissing) {
    CObj<NLSHead::CHeadSequenceLoader> loader = new NLSHead::CHeadSequenceLoader;
    loader->SetKey(id);
    if (loader->GetValue()) {
      std::fprintf(stderr, "missing sequence unexpectedly loaded id=%d\n", id);
      return 20;
    }
  }
  std::printf("sequence_refs_missing_loader_checked=%zu\n", selectedMissing.size());
  for (const auto &entry : package.Entries())
    if (!ids.count(entry.first)) {
      packageWithoutDB.insert(entry.first);
      std::printf("sequence_package_without_db id=%d\n", entry.first);
    }
  std::printf("sequence_corpus db=%zu present=%zu missing=%zu missing_digest=%016llX package=%zu package_without_db=%zu bytes=%zu tracks=%zu digest=%016llX\n",
    ids.size(), present, missing, static_cast<unsigned long long>(missingDigest),
    package.Entries().size(), packageWithoutDB.size(), bytes, tracksTotal,
    static_cast<unsigned long long>(digest));
  return ids.size() == 8359 && present == 6780 && missing == 1579 &&
    missingDigest == UINT64_C(0xBAE8D088A791CBD7) &&
    package.Entries().size() == 6782 &&
    packageWithoutDB == std::set<int>({6006, 6007}) &&
    bytes == 14204108 && tracksTotal == 41859 &&
    digest == UINT64_C(0x7F4692A1FFEBAD64) &&
    voiceIDs.size() == 5978 && HashIDs(voiceIDs) == UINT64_C(0x4B7B5467840BACF8) &&
    selectableVoiceIDs == voiceIDs &&
    expressionIDs.size() == 10 && HashIDs(expressionIDs) == UINT64_C(0xEB527EE7837C90AE) &&
    idleIDs.size() == 3 && HashIDs(idleIDs) == UINT64_C(0x09AFC47FBCAD4F5C) &&
    selectedIDs.size() == 5991 && HashIDs(selectedIDs) == UINT64_C(0xDEBF819ABD476152) &&
    selectedMissing.size() == 445 &&
    HashIDs(selectedMissing) == UINT64_C(0x368EA235718A571C) ? 0 : 19;
}

int main(int argc, char** argv) {
  if (argc != 3 && argc != 4) return 2;
  S2FileIO::PortablePackageIndex package;
  if (!package.Open(argv[1])) return 3;
  std::filesystem::current_path(std::filesystem::path(argv[2]).parent_path());
  NGScene::AddResourceDir(std::filesystem::path(argv[1]).parent_path().string().c_str());
  if (argc == 4) return CheckGameSequenceCorpus(package, argv[3]);
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
