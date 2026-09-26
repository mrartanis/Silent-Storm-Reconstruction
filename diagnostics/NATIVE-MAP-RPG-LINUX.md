# AI map and RPG mission compilation on Linux (stage 2)

The game-used `Main/aiMap.cpp` now compiles as `s2_game_ai_map` on Linux
GCC x86-64 and ARM64. The original `RPGGame.cpp`, `RPGStore.cpp`,
`RPGDiplomacy.cpp`, `RPGObject.cpp`, and `RPGUnitMission.cpp` compile as
`s2_game_rpg_mission`. The source adjustments handle case-sensitive
includes, the portable game RNG, valid enum declarations, and stderr
diagnostics on Linux while preserving the Windows paths and game rules.

The diplomacy bitfield had a real signed-left-shift UB at player index
15. `SDiplomacy` now shifts unsigned `DWORD` values. The new
`NativeDiplomacyBitsTests` executes the original methods across the
highest and a middle player slot, checking packed words `80008001` and
`00008001`. The test passed on restored Windows x86, Windows x64,
Linux x86-64, and ARM64/QEMU under ASan/UBSan with matching output.
The x86 run is a reference check, not a product build target.

This is **not** a runnable Linux map or mission. A diagnostic link of
the real `NativeAILogicTests` object with all available original game
archives still fails on 169 distinct unresolved symbols. `CreateAIMap`,
`CreateGame`, the store, diplomacy, and object creation are no longer
the direct missing symbols, but importing their bodies reveals further
combat, visibility, AI event, scenario/Lua, and render/world object
dependencies. No dummy implementations or unresolved-symbol bypass
were added. Runtime map navigation and combat parity with Steam remain
unverified.

Build with `cmake --build <linux-build> --target s2_game_ai_map
s2_game_rpg_mission NativeDiplomacyBitsTests -j 8`; run the focused test
with `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir <linux-build> -R
'^NativeDiplomacyBitsTests$' --output-on-failure`.
The complete Windows x64 `RelWithDebInfo` build passed 114/114 CTest
cases, and Linux GCC x86-64 and ARM64/QEMU passed 86/86 each with
`ASAN_OPTIONS=detect_leaks=0`.

A clean native-media Windows x64 archive
`G:\SS\lab\builds\stage2-map-rpg-20260926-01` was built with 16 jobs
from commit `07cc29b2f4fbedfa0b487170da4e1fd13fb5afb5`.
`Game.exe` SHA-256 is
`43DB83444C25388F109523186E00A8FAF2F47B4C23EC0904EE5061C162469A80`.
The archive has no FMOD DLL, and `Game.exe` imports neither `fmod.dll`
nor `FSOUND_`. In-game smoke remains unverified because the current
remote D3D session cannot create a device even for an older known-good
archive.
