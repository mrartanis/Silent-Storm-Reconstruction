# World gameplay package on Linux (stage 2)

The original `wBuilding.cpp`, `wTerrain.cpp`, `wInterface.cpp`,
`InventoryUnit.cpp`, `wDialog.cpp`, `wInformCorpseStop.cpp`, and
`wExplosionPerks.cpp` now compile in `s2_game_world_gameplay` on Linux
GCC x86-64 and ARM64. `RPGStatInfo.cpp` and `RPGBuilding.cpp` compile
in `s2_game_rpg_combat`. These are the game-used building, terrain,
inventory, dialogue, explosion-perk, and RPG-building paths; no editor
features were added to the portable core.

The Linux edits make precompiled-header prerequisites explicit,
normalize include paths, select the existing game RNG for building
behavior, and correct a signed/unsigned phrase-count comparison.
`Misc/HPTimer.cpp` now uses `std::chrono::steady_clock` nanoseconds on
non-Windows systems, so these native modules do not require x86 RDTSC
or Windows performance-counter APIs. The existing Windows calibration
path is unchanged.

Windows x64 `RelWithDebInfo` builds `Game.exe` and passes 116/116 CTest
cases. Linux GCC x86-64 and ARM64 build all declared targets and pass
90/90 CTest cases each under ASan/UBSan (ARM64 under QEMU). This package
does not add an executable test of live building destruction, dialogue,
or RPG object creation; the CTest result must not be read as proof of
those behaviors or Steam parity.

A diagnostic link of the real `NativeAILogicTests` object against all
current Linux game archives still fails with 15 distinct unresolved
symbols, down from 41 before this package. Remaining dependencies
include `GGeometry` helpers, FaceGen head construction, decals, Lua UI
registration, and several render/terrain vtables. No fake symbols or
unresolved-symbol linker bypass were used. The full Linux game and
live mission simulation are not executable yet. SDL3/bgfx integration
and the UI/render boundary are separate later work, not claimed here.

A clean native-media Windows x64 archive from source commit `01011e3`
was built with 16 jobs at
`G:\SS\lab\builds\stage2-world-gameplay-20260926-01`.
`Game.exe` SHA-256 is
`D5600D88BD0ADAEDC82690D8609B85DB931913585F3909C875FC5F19D737E107`.
The archive has no `fmod.dll`, and `Game.exe` has no `fmod.dll` or
`FSOUND_` imports. No in-game smoke is claimed from the current remote
D3D session.
