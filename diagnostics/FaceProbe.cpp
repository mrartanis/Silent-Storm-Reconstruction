// Deterministic LifeStudio vertex probe. Build for x86 (original DLL) and x64
// (native implementation), then compare the CSVs with Test-FaceParity.ps1.
#include <LifeStudioHeadAPIInit.h>
#include <LifeStudioHeadAPI.h>
#include <LifeStudioHeadAPIMMTS.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace LifeStudioHeadAPI;

static std::vector<char> ReadAll(const char *path)
{
  std::ifstream file(path, std::ios::binary);
  if (!file)
    return {};
  return std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

static bool WriteFrame(FILE *out, const std::vector<char> &animData,
                       IMMTree *tree, ISequencer *sequence, const char *label, int time)
{
  IAnimator *anim = IAnimator::Create();
  if (!anim)
    return false;
  bool ok = anim->Load(animData.data(), static_cast<int>(animData.size()));
  if (ok && tree)
    anim->RegisterMacroMuscle(tree->RootMacroMuscle());
  const int count = ok ? anim->VerticesCount() : 0;
  if (count <= 0 || count > 1000000)
    ok = false;
  std::vector<float> vertices(ok ? count * 3 : 0);
  if (ok)
  {
    anim->ClearAllMacroMuscles();
    if (sequence)
      sequence->RenderMacroMuscles(anim, time);
    anim->ComputePhysics();
    anim->FillUnused(true);
    ok = anim->Process(vertices.data(), 3);
  }
  if (ok)
    for (int i = 0; i < count; ++i)
      std::fprintf(out, "%s,%d,%d,%.9g,%.9g,%.9g\n", label, time, i,
                   vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
  anim->Destroy();
  return ok;
}

int main(int argc, char **argv)
{
  if (argc != 3 && argc != 4 && argc != 5 && argc != 6)
  {
    std::fprintf(stderr, "usage: FaceProbe animator.bin output.csv [sequence.bin tree.mma] [saved-animator.bin]\n");
    return 2;
  }
  const std::vector<char> animData = ReadAll(argv[1]);
  if (animData.empty())
  {
    std::fprintf(stderr, "cannot read animator: %s\n", argv[1]);
    return 2;
  }
  Init();
  const char *savedPath = argc == 4 ? argv[3] : argc == 6 ? argv[5] : nullptr;
  if (savedPath)
  {
    IAnimator *copy = IAnimator::Create();
    if (!copy || !copy->Load(animData.data(), static_cast<int>(animData.size())))
      return 3;
    const int size = copy->SaveBufferSize();
    std::vector<char> saved(size > 0 && size < 100000000 ? size : 0);
    const bool savedOk = !saved.empty() && copy->Save(saved.data());
    copy->Destroy();
    if (!savedOk)
      return 3;
    std::ofstream file(savedPath, std::ios::binary);
    file.write(saved.data(), saved.size());
    if (!file)
      return 2;
  }
  IMMTree *tree = nullptr;
  ISequencer *sequence = nullptr;
  int duration = 0;
  if (argc >= 5)
  {
    const std::vector<char> seqData = ReadAll(argv[3]);
    if (seqData.empty())
      return 2;
    tree = IMMTree::Create();
    sequence = ISequencer::Create();
    if (!tree || !sequence || !tree->Load(argv[4]) ||
        !sequence->Load(seqData.data(), static_cast<int>(seqData.size())))
    {
      std::fprintf(stderr, "cannot load sequence or muscle tree\n");
      if (sequence) sequence->Destroy();
      if (tree) tree->Destroy();
      return 3;
    }
    sequence->RegisterMMTree(tree);
    duration = sequence->SequenceTime();
    if (duration <= 0)
    {
      std::fprintf(stderr, "invalid sequence duration\n");
      sequence->Destroy();
      tree->Destroy();
      return 3;
    }
  }
  FILE *out = std::fopen(argv[2], "wb");
  if (!out)
    return 2;
  std::fprintf(out, "case,time,vertex,x,y,z\n");
  bool ok = WriteFrame(out, animData, tree, nullptr, "neutral", 0);
  if (ok && sequence)
  {
    std::vector<int> times = {0, duration / 4, duration / 2, 3 * duration / 4, duration - 1};
    std::sort(times.begin(), times.end());
    times.erase(std::unique(times.begin(), times.end()), times.end());
    for (int time : times)
      ok = WriteFrame(out, animData, tree, sequence, "sequence", time) && ok;
  }
  std::fclose(out);
  if (sequence) sequence->Destroy();
  if (tree) tree->Destroy();
  if (!ok)
  {
    std::fprintf(stderr, "LifeStudio Load/Process failed\n");
    return 4;
  }
  return 0;
}
