# Game-used 32-bit words on LP64 (stage 2)

Two Windows `unsigned long` fields in the non-graphics game core had become
64-bit on Linux x86-64 and ARM64. `CUnitArea::places` serializes its normalized
AI tile keys at tag 6; `CUnitServer::tDeathTime` serializes its game tick at
tag 35. The retail Windows contract is 32 bits for both. The generic
`StructureFieldCodec` treats arithmetic types as portable at their *host*
width, so a zero-raw-type wire audit could not detect this discrepancy.

`CUnitAreaPlaceKey` now uses `DWORD` (32-bit on every target), and both
`CUnitArea::Prepare` and `IsInArea` call the same normalized key helper.
`tDeathTime` now uses `STime`, the game's 32-bit tick type. Windows values
and behavior are unchanged; Linux no longer writes eight-byte words for
these fields. The change does not claim full AI decision parity.

`NativeUnitAreaWireTests` checks that tile `(0x55,0xaa)`, layer 3, standing
has key `0203AA55`; changing direction and moving bit does not change it,
while changing pose does. The one-entry map round-trips through the original
`CStructureSaver` with a 26-byte file and FNV-1a `A9A0A029C7FBC717`.
`NativeUnitDeathTimeWireTests` checks the actual member type is four bytes
and a test tick `89ABCDEF` round-trips at tag 35 with an 18-byte file and
FNV-1a `D132B87420B27244`. Both vectors match diagnostic Windows x86,
target Windows x64, Linux GCC x86-64, and ARM64/QEMU. Linux x86-64 ran with
ASan/UBSan/LSan; ARM64/QEMU with ASan/UBSan (LSan disabled under QEMU).

Build the two test targets with the normal CMake build for each platform;
run `ctest --test-dir <build> -C RelWithDebInfo -R
'^NativeUnit(Area|DeathTime)WireTests$' --output-on-failure` on Windows,
and the corresponding `ctest` command without `-C` on Linux. The Linux
builds also recompiled the original `s2_game_ai_actions` archive and the
linked `NativeAILogicTests`, which passed on both architectures under the
same sanitizer settings. Windows x64 rebuilt `Game.exe` and passed
`NativeAILogicTests`.

For the live world save/load check, run
`NativeWorldInitProbe <game.db> <res-dir> --mission-root-party-save-turn 5247
<scratch-save>` with `scripts/` in its working directory. Windows x64 and
Linux GCC x86-64 passed: party deployment, first save/load, hero turn,
ordinary end-turn command, second save/load, and continued updates. The
same focused mission passed on Linux GCC x86-64 under ASan/UBSan/LSan
and ARM64/QEMU under ASan/UBSan; ARM64 remains emulated, not physical hardware.
This is one authored mission, not proof of every serialized unit state or
every guard-area AI action. The focused wire tests verify the mapped field
types and scalar/map encoder, not a full `CUnitArea` object graph populated
by a live AI turn.
