#pragma once
#include "GLocale.h"
namespace NGScene {
CFontInfo* GetVectorFont(const SFont& font);
bool IsVectorTexture(CPtrFuncBase<NGfx::CTexture>* texture);
void ResetVectorFontDevice();
void ClearVectorFonts();
}
