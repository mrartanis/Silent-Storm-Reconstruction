# Shipped Lua window API and the stage-2 Linux boundary

The stage-2 portable core only needs game-used features. The original
`game.db` contains 113 `NDb::CScript` records (one empty, 355575 nonempty
bytes, current corpus digest `C2462A66D562BAF6` after the CP1251 import
correction documented in `NATIVE-GAME-DB-LINUX.md`).
`NativeMissionScriptCorpusTests` parses them with the modified Lua VM and
counts `OP_GETGLOBAL` reads. Of 1272 distinct global names, the eight
window-only API names are read six times total: `CreateWindow` three,
`GetWindow` once, and `ButtonCreateState` twice. All six reads occur in
script ID 126; the other five window API names are not read. The test
also checks references to ID 126 in the shipped `CUIContainer`,
`CTemplVariant`, `CGlobalMap`, `CChapterMap`, and `CRPGPers` tables and
finds zero. The four external baseline `.l` files contain no window API
calls. The hardcoded main-menu launcher uses script ID 85, not 126.

This is evidence that the window API is outside the currently known
shipped game path, not proof that ID 126 can never be invoked dynamically
or by a mod. We retain script 126 and the complete Windows implementation.
The eight globals now live in `scriptUI.cpp`'s `pUIRegList`, registered in
their original order after `pRegList` on Windows. The third Lua userdata
tag and its property tag methods likewise remain on Windows. Linux
headless core does not register those window-only functions and does not
pull `scriptUI.cpp` or its D3D9 dependency into gameplay Lua bindings.
If a required original-game path to ID 126 is found, revisit this split.

This change removes nine window-UI unresolved symbols from the
diagnostic link of the actual Linux game archives: the remaining six are
`CLightGroup` and `CNonePart` casting/registration, `CICShowObjectives`,
FaceGen `CHeadInfo` construction, and `NMainLoop::Command`. No dummy
implementation or unresolved-symbol linker bypass was introduced.
Other game-used UI commands remain in the portable Lua registration
table and still need a real headless/portable boundary. A linked,
running Linux mission and gameplay parity are **not** yet established.

Follow-up audit of interface globals: the shipped corpus reads
`ShowObjectives` zero times, but does read `uiShowStore` 5 times,
`uiShowTeamMngMenu` 3, `ShowLoseDialog` 3,
`ShowLeaveZoneDialog` 4, `ShowHint` 69, `AddHints` 2,
`SetTutorialMode` 1, `ClueShow` 11, and `ExitToChapter` 6.
The four external `.l` files also do not call `ShowObjectives`.
Its original implementation moved from the game scenario binding
unit into Windows `scriptUI.cpp`, and Windows still registers it.
No other interface global was removed. This narrows the diagnostic
link from six unresolved symbols to four: two `CNonePart` casts,
one `CLightGroup` cast, and the game-used FaceGen `CHeadInfo`
constructor. This is a boundary audit, not a substitute for
implementing the four dependencies or the game-used UI commands.

Validation on 2026-09-26: Windows x64 `Game.exe` builds and CTest is
118/118; Linux GCC x86-64 and ARM64/QEMU are each 92/92 under
ASan/UBSan.

Clean Windows x64 native-media archive from source commit `633c02c`:
`G:\SS\lab\builds\stage2-lua-used-surface-20260926-01`.
`Game.exe` SHA-256 is
`1DE073D4E0140642885288E703BF5FBFF2D459DB424C4EC81DD866210ED82411`.
The archive has no `fmod.dll`, and `Game.exe` has no `fmod.dll` or
`FSOUND_` imports. No in-game smoke was run for this package.

Clean follow-up archive from source commit `86268f0`:
`G:\SS\lab\builds\stage2-lua-interface-surface-20260926-01`.
`Game.exe` SHA-256 is
`BBE7945363A7413DF2F0F2D16E24735AA38F229A19667309A47FDD7F4611C6F9`.
It likewise has no `fmod.dll` file or `fmod.dll`/`FSOUND_` imports.
Windows x64 CTest passed 118/118, Linux x86-64 and ARM64/QEMU passed
92/92 each under ASan/UBSan. No in-game smoke was run for this follow-up.
