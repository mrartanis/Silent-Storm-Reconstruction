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

The follow-up Linux `LSHeadPortable.cpp` now supplies the actual
game-used lazy `CHeadMeshLoader`, `CHeadSequenceLoader`, shared head cache,
and `CHeadInfo(NDb::CComplexHead*)` constructor, with their original
save/load class IDs. The resource test constructs both loaders on Linux,
then loads original `game.db`, chooses the lowest valid complex-head ID,
constructs `CHeadInfo`, forces its mesh, and saves/reloads the object.
Windows x64, Linux x86-64, and ARM64 select complex head 1, source head
6, one animator, and a 182-byte serialized `CHeadInfo`. This is a real
database-to-head-to-resource-to-animation-to-save path, not a factory
that only satisfies a linker symbol. The Linux implementation uses the
same resource fields and native LifeStudio algorithms as Windows x64,
although the old `LSHead.cpp` remains the Windows compilation unit.

Re-linking the actual Linux AI-logic object with all available game
archives now leaves three distinct unresolved scene symbols: two
`CNonePart` casts and one `CLightGroup` cast. The previous fourth,
`CHeadInfo` construction, is resolved. This diagnostic link is not a
Linux game executable; scene ownership/serialization and a live mission
remain open. A direct Steam x86 comparison for the selected head save
or all facial vertices has not been run in this packet.
The full regression matrix after this packet passed on 2026-09-26:
Windows x64 122/122, Linux x86-64 and ARM64/QEMU 96/96 each under
ASan/UBSan. No Windows game source changed, so no new Windows game
archive was made for this Linux-runtime packet.

Reproduce on Windows with the configured `G:\SS\lab\build-x64-stage2`:

```
cmake --build G:\SS\lab\build-x64-stage2 --config RelWithDebInfo --target NativeHeadResourceTests --parallel 16
ctest --test-dir G:\SS\lab\build-x64-stage2 -C RelWithDebInfo -R ^NativeHeadResourceTests$ -V
```

On Linux, configure with
`-DS2_HEAD_RESOURCE_PATH=/path/to/original/res/Heads.res` and, when
`tree.mma` is not in the parent of `res`, set
`-DS2_HEAD_TREE_PATH=/path/to/original/tree.mma`. Set
`-DS2_GAME_DB_PATH=/path/to/original/game.db` to include the
`CHeadInfo` constructor and save/reload check. Build the
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

## Game-linked sequence corpus follow-up (2026-09-28)

The original three-ID `NativeHeadSequenceRuntimeTests` remains a quick
blink/speech/expression playback regression. Its optional fourth argument,
`<game.db>`, now runs the bounded source-to-payload audit:

```text
NativeHeadSequenceRuntimeTests <Sequences.res> <tree.mma> <game.db>
```

The game's `CSequence` table contains 8,359 IDs. Of these, 6,780 have an
effective `Sequences` resource and all 6,780 pass strict envelope/track
decoding and the actual `CHeadSequenceLoader`/`ISequencer` load. The test
hashes sorted IDs, semantic duration/track counts, and all selected stream
bytes: 14,204,108 bytes, 41,859 tracks, digest `7F4692A1FFEBAD64`. No
loose `res/Sequences` override directory exists in this base install.
`Sequences.res` has 6,782 entries: only 6006 and 6007 lack a DB record.

The other 1,579 DB IDs lack payloads in the original package (ID-set digest
`BAE8D088A791CBD7`). This is not a cross-platform read failure. The
game's `CDBAckInfo::GetVoice` can select 5,978 distinct sequence IDs;
445 of those lack a file (digest `368EA235718A571C`). All 445 are exercised
through the game's lazy loader and return no sequence instead of crashing.
The ten facial-expression IDs and three ambient-idle IDs all have files.
An absent voice sequence can therefore mean no lip animation for that
authored phrase; the test does not claim that the missing content exists or
that every such phrase is observed in a mission.

Windows x64, Linux GCC x86-64 and ARM64/QEMU under ASan+UBSan match these
counts and digests, with no sanitizer diagnostic. The ordinary three-ID
CTest also passes on all three after this change. The
earlier x86-versus-x64 [face corpus](FACE-PARITY.md) exported these 6,780
game sequences from the reconstructed x86 loader and compared sampled
vertex results on one saved head; it is a separate behavioral oracle, not
an x86 run of this new resource-hash test. The input `Sequences.res`,
`game.db`, and `tree.mma` SHA-256 values are respectively
`52C2C7133766AD42B46B9EC8C74874CC4CA9895562C25673C08529CB1EABBACB`,
`314D7AA9E6339F0E6826BF2A0C71FDAD992AEC1D6074A2ED763A6F6FA62D62DF`,
and `F711562E7532667E94F2F0AE01672C574F969585B5658D760494D519A56C5E12`.
This is CPU data/loading coverage, not a render or every-frame parity claim.
