# Scene object serialization in the stage-2 core

The original `GSceneUtils.cpp` now compiles on Linux GCC x86-64 and
ARM64. `CDecalTarget`'s real save/load registration was moved from
`GDecal.cpp` into `GDecalTarget.cpp`, shared by the Windows game and
Linux `s2_game_scene_serialization` target. The diagnostic particle
vector was likewise moved without behavior changes from `RWGame.cpp`
to `DebugParticles.cpp`, retaining one shared definition for game AI,
animation, and the Windows renderer.

`NativeSceneClassIDsTests` checks the original `CCInt` class ID
`0x03031620` and a write/read round-trip through `CStructureSaver`.
The serialized stream is 43 bytes with FNV-1a
`83668957BCAD397D` on restored Windows x86, Windows x64, Linux
x86-64, and ARM64/QEMU. This is a focused object-wire regression, not
a whole-save compatibility claim. The `CDecalTarget` registration
compiles, but a live decal target containing scene parts still depends
on renderer-owned `CNonePart` and has no portable round-trip test.

Windows x64 `RelWithDebInfo` builds `Game.exe` and passes 118/118 CTest
cases. Linux GCC x86-64 and ARM64 build all targets and pass 92/92
tests each under ASan/UBSan (ARM64 under QEMU).

The diagnostic link of the real `NativeAILogicTests` object against
all current Linux game archives still fails on 15 distinct unresolved
symbols, down from 17 before this package. Closing the simple `CCInt`,
`CDecalTarget`, and particle symbols exposes `CNonePart` references;
light groups, FaceGen head construction, mission UI commands, and
window Lua methods are also unresolved. No fake objects or linker
bypasses were added. Full game saves and live Linux missions remain
unverified.

A clean native-media Windows x64 archive from source commit `32e4d91`
was built with 16 jobs at
`G:\SS\lab\builds\stage2-scene-serialization-20260926-01`.
`Game.exe` SHA-256 is
`A8D7484854A6AD55BCF90782C9590FA24A9A12CF3C01CAD955F3D9A2BC97F7A2`.
The archive has no `fmod.dll`, and `Game.exe` has no `fmod.dll` or
`FSOUND_` imports. No in-game smoke is claimed from the current remote
D3D session.
