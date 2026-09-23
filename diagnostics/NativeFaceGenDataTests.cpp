#include "NativeFaceGenData.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

int main()
{
  const std::string valid =
      "SLIDER Age -100 100\n"
      "HEAD Age=-25 Gender=100 African=100 Asian=-100 Arab=-100 One\n"
      "HEAD Age=-25 Gender=100 African=-100 Asian=100 Arab=-100 Two\n"
      "HEAD Age=-25 Gender=100 African=-100 Asian=-100 Arab=100 Three\n"
      "COMB Age=-25 Gender=100 African=100 Asian=100 Arab=-100 One=50 Two\n"
      "COMB Age=-25 Gender=100 African=100 Asian=100 Arab=100 One=33 Two=33 Three\n";
  NativeLifeStudio::FaceGenData data;
  std::vector<char> links(12 + 3 * 64 + 2, 0);
  const unsigned char header[] = {0x8e, 0x11, 0xd7, 0x22, 2, 0, 0, 0, 1, 0, 0, 0};
  std::memcpy(links.data(), header, sizeof(header));
  std::memcpy(links.data() + 12, "MorphA", 6);
  std::memcpy(links.data() + 76, "MorphB", 6);
  std::memcpy(links.data() + 140, "Output", 6);
  links[links.size() - 2] = 7;
  NativeLifeStudio::FaceGenLinks decoded;
  const bool ok = NativeLifeStudio::ParseFaceGenRules(valid, &data) &&
      data.heads.size() == 3 && data.combinations.size() == 2 &&
      data.combinations[0].parts.size() == 2 &&
      data.combinations[0].parts[0].percent == 50 &&
      data.combinations[0].parts[1].percent == 50 &&
      data.combinations[1].parts[2].percent == 34 &&
      data.heads[0].parameters[0] == -25 &&
      data.heads[0].parameters[2] == 100 &&
      !NativeLifeStudio::ParseFaceGenRules(
          "HEAD Age=-25 Gender=100 African=100 Asian=-100 Arab=-100 Bad/Name\n", &data) &&
      !NativeLifeStudio::ParseFaceGenRules(valid +
          "COMB Age=0 Gender=0 African=0 Asian=0 Arab=0 One=99 Unknown\n", &data) &&
      !NativeLifeStudio::ParseFaceGenRules(valid +
          "COMB Age=0 Gender=0 African=0 Asian=0 Arab=0 One=100 Two\n", &data) &&
      !NativeLifeStudio::ParseFaceGenRules(valid +
          "COMB Age=0 Gender=0 African=0 Asian=0 Arab=0 One Two\n", &data) &&
      NativeLifeStudio::DecodeFaceGenLinks(links.data(), links.size(), &decoded) &&
      decoded.morphNames.size() == 2 && decoded.outputNames.size() == 1 &&
      decoded.morphNames[0] == "MorphA" && decoded.outputNames[0] == "Output" &&
      decoded.matrix.size() == 2 && decoded.matrix[0] == 7 &&
      !NativeLifeStudio::DecodeFaceGenLinks(links.data(), links.size() - 1, &decoded);
  if (!ok) std::fprintf(stderr, "FaceGen rule parser regression\n");
  return ok ? 0 : 1;
}
