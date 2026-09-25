# Original Lua runtime on Linux and ARM64

Stage 2 now builds the game's modified Lua 4 parser, VM, thread scheduler,
function registry, and `Script` wrapper from the **original `Script/` sources**
into `s2_game_lua` on Linux x86-64 and ARM64. This is runtime execution, not
merely decoding a saved Lua record. The native Windows `Script` target still
uses the same source files. `Instruction` and `lint32` are fixed 32-bit words
on all hosts; `IntPoint` hashes full-width pointers rather than truncating
them to Windows `unsigned long`. `NativeLuaWordTests` obtains identical
little-endian instruction hash `292565a5d957c367` on Windows x64, Linux
x86-64, and ARM64/QEMU.

`NativeLuaRuntimeTests` executes a Lua chunk through the game's thread
scheduler (result `96`), then executes another through the `Script` wrapper
(result `39`). With `--autoload SCRIPT_DIR`, it queues the four original
startup scripts in game order, runs 25 scheduler frames with the standard
`Sleep` and `StartThread` functions, and checks selected game constants,
trigger state, and four `out` calls. Each script also passed the parser-only
probe. The licensed files stay in the lab, not the repository:

| Original script | Bytes | SHA-256 |
| --- | ---: | --- |
| `Constants.l` | 1,493 | `9d1cf1a9a11aafc49d0a2b5078e7828dedf0f6c84ba5776af4763d81700c9a80` |
| `TriggersManager.l` | 1,108 | `1c8f517ff2205fd74c076bc87a2ecf3f8238a0bab4092629ff20d59d9bd5ec30` |
| `Common.l` | 15,196 | `2786c9a37bbf6d14c285dcd2f3e7b0c33ad8291c5c75fb2949804f3099655727` |
| `Hint.l` | 101 | `3c8608e8c2b3b91724662862cc08794adda78c1af514c7ac77507b9333d65568` |

To include this resource-dependent regression in CTest, configure with
`-DS2_SCRIPT_CORPUS_DIR=<directory containing these four .l files>` and run
`ctest --test-dir BUILD_DIR -R '^NativeLua' --output-on-failure`. Linux builds
use `-fsanitize=address,undefined`; under ARM64/QEMU set
`ASAN_OPTIONS=detect_leaks=0` because LeakSanitizer cannot operate under
QEMU's ptrace-based execution. The runtime test also proves that attempting
Lua state serialization on Linux throws instead of writing a fake save.

Verified 2026-09-25: Windows x64 CTest 84/84, Linux x86-64 and ARM64/QEMU
51/51 each, including the autoload corpus. These checks do **not** close
stage 2: most game-specific Lua bindings in `Main/` and the world/AI/combat
loop still have no Linux target. The Linux `CStructureSaver` bridge explicitly
rejects persistence; Lua state save/load is not yet ported. No Linux
`Game.exe` or playable mission is claimed.
