# Game Lua bindings and script-controlled AI (stage 2)

The original `ScriptFunctions.cpp` registration table and the game
binding units `scriptDialog.cpp`, `scriptDiplomacy.cpp`,
`scriptObject.cpp`, `scriptPosition.cpp`, `scriptRoute.cpp`,
`scriptScenario.cpp`, `scriptSequence.cpp`, `scriptTemplate.cpp`,
`scriptUnit.cpp`, and `scriptUnitGroup.cpp` now compile in
`s2_game_script_bindings` on Linux GCC x86-64 and ARM64. The original
`aiScriptLogic.cpp` and `aiScriptReaction.cpp` compile in
`s2_game_ai_actions`; these are the script-controlled unit AI paths.

The edits make Windows-PCH prerequisites explicit, normalize include
case and separators, name the database pointer dependency explicitly,
and correct non-portable conversions from dynamic-cast wrappers to
retaining pointers. `luaUnitApplyCritical` now passes a live local
parameter vector to `luaPrepareData` instead of taking the address of
a temporary. These changes preserve the intended game operations.

Windows x64 `RelWithDebInfo` builds `Game.exe` and passes 117/117 CTest
cases. Linux GCC x86-64 and ARM64 build all targets and pass 91/91
tests each under ASan/UBSan (ARM64 under QEMU).
The existing mission-script corpus test reads original script resource
bytes; it does not execute these new bindings. No gameplay parity claim
follows from the compilation and this test matrix alone.

The diagnostic link of the real `NativeAILogicTests` object against all
current Linux game archives still fails on 17 distinct unresolved
symbols. The previous count of eight did not include the Lua binding
table, so this increase exposes real transitive dependencies rather
than a regression. Remaining symbols include UI commands and the eight
window Lua functions, FaceGen head construction, scene/decal
serialization, and debug particle storage. `scriptUI.cpp` itself hits
`D3D9.h` when compiled on Linux; that is the existing graphical UI
boundary. No dummy UI functions or unresolved-symbol linker bypass
were added. Full Lua mission execution and the Linux game are not yet
available.
