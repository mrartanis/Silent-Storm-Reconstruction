# Shipped Lua window API and the stage-2 Linux boundary

The stage-2 portable core only needs game-used features. The original
`game.db` contains 113 `NDb::CScript` records (one empty, 355575 nonempty
bytes, corpus digest `B5163E4E76664106`).
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

Validation on 2026-09-26: Windows x64 `Game.exe` builds and CTest is
118/118; Linux GCC x86-64 and ARM64/QEMU are each 92/92 under
ASan/UBSan.
