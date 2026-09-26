# Original head resources in the stage-2 portable data path

`NativeHeadResourceTests` reads the shipped `Heads.res` through the
engine's `CResourceOpener` and `CStructureSaver`, using the same tags and
field types as `CHeadMeshLoader::Recalc`: animator streams, segment
vertex counts, copied vertices, UVs, 16-bit indices, and triangles.
It then decodes every LifeStudio animator stream with the existing
portable `NativeLifeStudio::DecodeHeadVertices` implementation. The
original package contains 134 head records and 136 animator streams;
all decode. They contain 56,462 vertices and 4,825 muscles. A digest
over resource IDs, topology, UVs, vertex positions, stored muscle
weights, names, and bones is `8A6A6F3A612C18D0` on Windows x64,
Linux x86-64, and ARM64/QEMU. Derived falloff weights are checked
finite/in-range and hashed to 0.001 precision: native ARM64 floating
point differs by a few ULPs from x64, so they are not claimed to be
bit-identical.
The test asserts these values, not merely successful file opening.
The audit also found that the compact saved-head stream omits the
implicit vertex's `type` and `attribute`; the decoder previously left
them uninitialized. They now default to zero, with a focused
`NativeHeadDataTests` assertion.

Follow-up: the six-tag read now lives in `Main/HeadResourceData.cpp`.
The Windows game's real `CHeadMeshLoader::Recalc` consumes its result
before constructing LifeStudio animators; the same function is called
by `NativeHeadResourceTests` on Windows/Linux. On Windows the test also
constructs the live lazy loader for head ID 11 and checks its animator
count and all five geometry vectors against the shared CPU result.
The Windows loader behavior is retained, not replaced with a test-only
parser. At this point Linux used only the shared CPU fields.
After this integration, the full matrix again passed: Windows x64
119/119, Linux x86-64 and ARM64/QEMU 93/93 each under ASan/UBSan.

The test uses the real engine resource/structure loading path and
original data. This earlier version did not construct `CHeadInfo`,
initialize LifeStudio animators, transform a head, render a face, or
exercise a live game mission. In particular, the four diagnostic Linux link
symbols (`CNonePart` twice, `CLightGroup`, and `CHeadInfo` construction)
are not resolved by this resource test. The renderer-owned scene-part
classes and head cache must retain their actual behavior when brought
into the portable kernel; a registration-only substitute would not do.

The next packet compiles the existing native LifeStudio `IAnimator`,
`IMMTree`, and `ISequencer` implementations as `s2_native_lifestudio` on
Linux. `NativeHeadResourceTests` now loads head 11's first original
animator stream into the same implementation on Windows x64 and Linux
x86-64/ARM64, checks vertex/muscle/bone counts, round-trips the stream,
and executes neutral `Process`. The coarse (0.01 unit) coordinate hash
is `ED804368E6F1CACA` on all three. The finer 0.0001-unit hash is
`D5E909ECB88BC4C1` on Windows/Linux x64 and `253CD0F88D3405B2` on
ARM64; the absolute-coordinate sums are 4768.274680 versus
4768.274693. This is a measured floating-point difference, not a
claim of exact ARM64 vertex parity.

`NativeHeadSequenceRuntimeTests` reads original `Sequences.res` through
the engine opener and loads game-used blink 6005, speech 371, and held
expression 7552 into `ISequencer`. Durations/tracks match the portable
decoder: 18000/8, 10000/244, and 1002/8. It loads the original
`tree.mma`, registers its root macro muscle with head 11, and applies
expression 7552 at time 500. All three platforms report 712 changed
head coordinates. This exercises original resource loading and the
native CPU animation path; it does not establish per-vertex parity
against Steam x86, link `CHeadInfo` on Linux, render a face, or run a
mission. The native LifeStudio implementation remains partial in other
API surfaces, so Linux FaceGen is not declared complete.

Reproduce on Windows with the configured `G:\SS\lab\build-x64-stage2`:

```
cmake --build G:\SS\lab\build-x64-stage2 --config RelWithDebInfo --target NativeHeadResourceTests --parallel 16
ctest --test-dir G:\SS\lab\build-x64-stage2 -C RelWithDebInfo -R ^NativeHeadResourceTests$ -V
```

On Linux, configure with
`-DS2_HEAD_RESOURCE_PATH=/path/to/original/res/Heads.res` and, when
`tree.mma` is not in the parent of `res`, set
`-DS2_HEAD_TREE_PATH=/path/to/original/tree.mma`. Build the
`NativeHeadResourceTests` and `NativeHeadSequenceRuntimeTests` targets,
then run `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-x64 -R
'NativeHead(Resource|SequenceRuntime)Tests' -V`. The ARM64 configuration
uses the same resources and tests via QEMU.

Full regression matrix on 2026-09-26: Windows x64 119/119;
Linux x86-64 and ARM64/QEMU 93/93 each under ASan/UBSan.
After the native LifeStudio runtime and sequence packet: Windows x64
122/122, Linux x86-64 and ARM64/QEMU 96/96 each under ASan/UBSan.
The Windows game source was not changed by this test/runtime package;
the clean game archive recorded below predates it.

Clean native-media Windows x64 archive from source commit `12de5d0`:
`G:\SS\lab\builds\stage2-head-resources-20260926-01`.
`Game.exe` SHA-256 is
`774F6C5E4AAD58A6155A4AA8B67E8ECB538F17AF814D8E9A397448B8B7EE1E87`.
The archive has no `fmod.dll`, and `Game.exe` has no `fmod.dll` or
`FSOUND_` imports. No in-game smoke is claimed for this archive.

Clean Windows x64 archive after the shared-loader integration, from
source commit `e57ee88`:
`G:\SS\lab\builds\stage2-shared-head-loader-20260926-01`.
`Game.exe` SHA-256 is
`1BB071F569B0A69EA9F5EBD03C1EBC65D43E54A4433733BDCA66F0B859EC1E45`.
There is no `fmod.dll` file or `fmod.dll`/`FSOUND_` import. No live-game
smoke is claimed for this archive; the in-process lazy-loader test is
the direct game-code check for this package.
