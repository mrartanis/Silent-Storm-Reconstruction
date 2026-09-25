# Original path-network compile boundary

Stage 2 now compiles the game's original `Main/aiGrid.cpp` translation unit
as `s2_game_ai_grid` on Linux GCC x86-64/ARM64 and Clang x86-64. This is a
compile gate, not yet a runnable Linux `CPathNetwork` or mission-routing
claim. The source is included in the complete headless build so later
portable-header changes cannot silently break it.

The compile port replaced the Windows precompiled-header dependency with
the existing headless platform/structure/geometry headers, normalized
game-used include paths and case, made the original object-refcount macro
valid for template classes, and made dependent iterator/base names explicit
for GCC/Clang. Forward-declared enums used by the world interface now have
fixed `int` underlying types (their original runtime size); `aiGrid.cpp`'s
debug output uses portable formatting on Linux. The transient `SMove`
non-Windows representation and `STempArray<T>` macro route are documented
in `NATIVE-HEIGHT-LAYERS-LINUX.md`.

Clang exposed a one-before-array pointer in the original `CPool<T>` block
cursor. `Pool.h` now uses a next-free pointer and a legal one-past-end
pointer; `NativePoolTests` allocates across three blocks, iterates back
over each boundary, clears, and reuses the pool. This code is used by the
AI renderer and other game systems and is not a serialized layout.

A direct link attempt between `s2_game_ai_grid` and
`NativeHeightLayersTests` establishes the next required runtime modules:
original pass/move calculators, AI position and job helpers, lock handling,
logging, and mine RTTI. The attempted link failed on those unresolved
original symbols; it was not replaced with fake definitions. Accordingly
the height-layer test still discards the `ComputeLayers` path-network
branch as described in `NATIVE-HEIGHT-LAYERS-LINUX.md`. Full execution of
that branch remains required before claiming building-aware floor heights.

On the authorized Linux host, after syncing the repository source:

```sh
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-x64 -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-x64 --target s2_game_ai_grid NativePoolTests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 -R NativePoolTests --output-on-failure
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-arm64 -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-arm64 --target s2_game_ai_grid NativePoolTests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 -R NativePoolTests --output-on-failure
export CPLUS_INCLUDE_PATH=/usr/include/c++/11:/usr/include/x86_64-linux-gnu/c++/11:/usr/include/c++/11/backward
export LIBRARY_PATH=/usr/lib/gcc/x86_64-linux-gnu/11
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-clang-release -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-clang-release --target s2_game_ai_grid NativePoolTests -j 16
ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-clang-release -R NativePoolTests --output-on-failure
```

Windows `NativePoolTests` is built with the original `Main` include
environment; the full Windows x64 `Game` target and CTest suite remain the
regression gate for the shared headers. The Windows x64 game rebuilt and
91/91 CTest passed; the x86 `NativePoolTests` also passed. Linux GCC x86-64
and ARM64 sanitizer suites and Clang x86-64 each passed 64/64. As
elsewhere, the recovered x86 build is a supplemental implementation
comparator, not Steam itself.

Clean native-media x64 archive
`G:\SS\lab\builds\stage2-ai-grid-compile-20260925-01` was produced
from source commit `79cc6e9`. Fresh linked-resource run
`G:\SS\lab\runs\stage2-ai-grid-compile-clean-01` had no `fmod.dll`,
loaded the existing `DB_OLD` slot to `LOAD-SLOT-DONE`, accepted `quit`,
and exited without a crash dump. Its `Game.exe` SHA-256 matched the archive:
`C70F04B1AFA3A7A985FD37733778B8E426C0DA20DD8C9015B75AA8CEAE9C3A4B`.
This checks the Windows x64 game after common-header edits, not Linux
`CPathNetwork` execution.
