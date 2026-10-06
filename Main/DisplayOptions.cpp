#include "StdAfx.h"
#include "DisplayOptions.h"
#include "GInit.h"
#include "../Game/Platform.h"
#include "../MiscDll/Commands.h"
#include "../MiscDll/LogStream.h"
namespace {
bool pending = false;
wstring previousResolution;
float previousFullscreen = 0;
std::uint32_t deadline = 0;
}
namespace NGScene {
void BeginDisplayChange() {
  if (pending) RevertDisplayChange();
  previousResolution = NGlobal::GetVar("gfx_resolution", 1024).GetString();
  previousFullscreen = NGlobal::GetVar("gfx_fullscreen", 1).GetFloat();
  // Remember the actual window size when switching into fullscreen mode.
  if (previousFullscreen == 0) {
    const auto& m = S2Platform::Display();
    WCHAR text[64]; swprintf(text,64,L"%dx%d",m.windowWidth,m.windowHeight);
    previousResolution = text;
    NGlobal::SetVar("gfx_resolution", previousResolution);
  }
  deadline = S2Platform::Milliseconds() + 15000;
  pending = true;
}
void ConfirmDisplayChange() { pending = false; NGlobal::SaveConfig(".\\cfg\\config.cfg"); }
void RevertDisplayChange() {
  if (!pending) return;
  pending = false;
  NGlobal::SetVar("gfx_resolution", previousResolution);
  NGlobal::SetVar("gfx_fullscreen", previousFullscreen);
  if (!SetModeFromConfig(false)) csSystem << "DISPLAY: previous mode could not be restored" << endl;
}
int DisplayChangeSeconds() {
  if (!pending) return 0;
  const auto remaining = static_cast<std::int32_t>(deadline - S2Platform::Milliseconds());
  return remaining > 0 ? (remaining + 999) / 1000 : 0;
}
void PollDisplayChange() { if (pending && !DisplayChangeSeconds()) RevertDisplayChange(); }
}
