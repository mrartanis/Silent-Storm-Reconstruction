#include "NativeFaceGenData.h"
#include <cstdio>
#include <string>

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
          "COMB Age=0 Gender=0 African=0 Asian=0 Arab=0 One Two\n", &data);
  if (!ok) std::fprintf(stderr, "FaceGen rule parser regression\n");
  return ok ? 0 : 1;
}
