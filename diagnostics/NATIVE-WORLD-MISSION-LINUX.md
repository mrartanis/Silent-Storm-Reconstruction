# Native mission creation and post-init (stage 2)

`NativeWorldInitProbe` has an optional `--mission <variant>` path. It loads the
original `game.db` and four autoload scripts, constructs the original `CWorld`
with an RPG global game, calls `CWorld::CreateRandom` with the original
`BuildMap`, then calls `CWorld::RunPostInit`. Variant 218 exercises a small
building map with no units or attached script. Variant 810 adds two authored
units and one 1,016-byte attached Lua script; its startup also runs the
world's first-segment warm-up, vision, physics, and path-colouring jobs.

The tests are `NativeWorldMission218` and `NativeWorldMission810` in both the
Windows and portable CMake builds. Example Linux verification, from a build
configured with `S2_GAME_DB_PATH`, `S2_RESOURCE_PACKAGE_PATH`, and
`S2_SCRIPT_CORPUS_DIR`:

```sh
cmake --build build-x64 --target NativeWorldInitProbe NativeBuildingGridTests NativeHeightNetworkTests -j 16
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
  ctest --test-dir build-x64 --output-on-failure \
  -R 'NativeWorld(Mission|Init)|NativeBuildingGridTests|NativeHeightNetworkTests'
```

The data package and Lua files must be the same originals used by the other
stage-2 resource probes. The Linux test stages the autoload files under its
working directory; resource packages remain in the configured resource
directory. `ASAN_OPTIONS=detect_leaks=0` is the existing QEMU-compatible
sanitizer setting; UBSan's `halt_on_error=1` is intentional for this gate.

The final packet passed the full CTest matrix on 2026-09-26: Windows x64
129/129, Linux x86-64 106/106, and Linux ARM64/QEMU 106/106. Both Linux
suites used ASan/UBSan, `ASAN_OPTIONS=detect_leaks=0`, and
`UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1`.

The first live variant-810 run exposed three previously dormant x64/Linux
undefined behaviors, now covered by the mission gate and local regressions:
`SFlipper` moved an uninitialized `bool` during path-network growth;
building-junction hashes shifted negative signed coordinates; and AI-map hull
queries called methods on absent octree children. The fixed-point vision
rasterizer also relied on signed 32-bit overflow for edge stepping; its
unsigned modulo-32 arithmetic now states the original wraparound explicitly.
The coordinate hashes retain their original signed `int` return values, so
the 64-bit hash table sees the same sign extension as before.

The probe now restores the active Lua stack after reading startup globals.
Without that guard, the diagnostic itself left the numeric constant `7` in
the `TriggersManager` thread's call slot; its later `Sleep(10)` then produced
"attempt to call a number value" even on a script-free map. This was a
probe-induced failure, not evidence of a game Lua defect. After the guard,
both map variants advance 220 world segments without a Lua error. For variant
810 the probe first forces NIGHT and verifies that the authored script's
`SetTimeOfDay(DAY)` changes the world back to DAY during post-init. This is a
specific observed mission-script effect, not merely successful parsing.

This is a headless original-world startup, **not** a full Linux game. The
probe does not create the mission UI, deploy a human party, issue a combat
command, save/reload the resulting mission, or compare dynamic AI, route,
battle, and destruction decisions against Steam x86. The variant-810 script
also waits on sequence/UI actions; the headless probe does not prove that its
later callbacks or unit-shot command complete. The absence of Lua errors and
the time-of-day effect do not prove that every mission binding worked.
Those are the next stage-2 checks. SDL3/bgfx integration remains in stages
3-4, and no macOS result is claimed.
