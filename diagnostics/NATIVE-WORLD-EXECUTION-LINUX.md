# World execution, projectiles, and entities on Linux (stage 2)

The game-used `wMainPath.cpp`, `wUnitAttack.cpp`, `wUnitExec.cpp`,
`wUnitMove.cpp`, `wUnitQueue.cpp`, and `wUnitAttackExec.cpp` compile on
Linux GCC x86-64/ARM64 as the original path and unit-command layer.
`RPGBullet.cpp`, `aiSmoothPath.cpp`, `wHintsFunc.cpp`, `wMisc.cpp`,
`wDebris.cpp`, `wMine.cpp`, and `wObject.cpp` compile as the adjacent
projectile, path, sound, and world-entity layer. These are static
archives, not an executable Linux mission.

Portability edits make PCH prerequisites explicit, normalize
case-sensitive includes, preserve the game's 32-bit command-result
enum, and select the existing game RNG on Linux where libc's `random`
collides with the Windows global. `RPGBullet.h` now names the actual
`A5Time.h` that defines `STime`. A pure melee-reach gate was moved
unchanged from `wUnitAttackExec.cpp` to `wHumanReach.cpp`, included in
both the Windows game manifest and the Linux target. The
`NativeHumanReachTests` executable checks horizontal and vertical
boundaries of that same implementation.

Windows x64 `RelWithDebInfo` builds `Game.exe` and passes 116/116
CTest cases. Linux GCC x86-64 and ARM64/QEMU build all declared targets
and pass 90/90 tests each under ASan/UBSan. The auxiliary restored-x86
reach test passes 1/1. A diagnostic link of
the real `NativeAILogicTests` object against all available Linux game
archives still fails on 66 distinct unresolved symbols, down from
118 before this connected world group. The remaining references
include buildings/terrain, event and unit objects, bullet servers,
dialog/Lua hooks, UI/render bindings, and registration casts. No
dummy implementation or unresolved-symbol linker bypass is used.

The narrow reach test is not proof of combat parity. The x86 restored
build is only an auxiliary rules comparison; the shipped Steam EXE
remains the behavioral reference. There is no in-game smoke for this
batch because the current remote Windows D3D session cannot create a
device even for an older known-good archive.
