# Game particle-effect runtime data (stage 2)

`GView` requests `CParticlesLoader` for DB effect IDs. The loader reads
`Effects.res`, converts its position, rotation, scale, color and sprite
keys to aligned runtime tracks, and retains the lazy file request. The
portable byte decoder existed, but the original game loader had not been
compiled or executed on Linux. `Main/GParticleFormat.cpp` is now in the
portable scene-data target, with the same code still compiled by the
Windows game.

`NativeParticleRuntimeResourceTests` starts the game's resource-loading
thread, requests every entry in the shipped `Effects.res` through the
original `CParticlesLoader`, and hashes all returned fields. For each
entry it separately decodes the package payload with the shared wire
parser and requires its semantic hash to equal the runtime object's
hash. This checks the runtime conversion rather than only the raw
package decoder. The
complete corpus has 280 effects, 94,612 particles, 1,917,803 keys and
FNV-64 `89E2A0AF09D801B5`. Diagnostic Windows x86, target Windows x64,
Linux GCC/Clang x86-64 and ARM64/QEMU agree. GCC x86-64 uses
ASan/UBSan/LSan; ARM64/QEMU uses ASan/UBSan. The full Windows x64 CTest
passed 169/169; ordinary Linux GCC x86-64 CTest passed 138/138.

Build `NativeParticleRuntimeResourceTests` and run its registered CTest
case with the original game directory configured, or pass
`<res-directory>/Effects.res` to the executable directly. This is a
CPU/lazy-loader data test. It does not exercise GPU particles, textures,
lighting, effect timing in a live mission, or graphical parity; those
belong to the rendering stage. Windows x86 is diagnostic only.
