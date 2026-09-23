# LifeStudio x86 → native x64 parity

Status: the x86 reference is executable and deterministic. The native x64
bridge passes strict animated vertex parity on the initial 3-head × 2-sequence
set and on the 20-sequence representative set for head 56. This is bounded
corpus parity, **not** a claim that all facial animation or the live portrait
is complete.

`FaceProbe` exercises the same `IAnimator`, `IMMTree` and `ISequencer` calls as
`NLSHead::CHeadAnimator`: load a real animator stream, register `tree.mma`,
clear/render macro-muscles, compute physics, fill unused vertices and process
positions. It exports the neutral pose and five evenly spaced samples through
one sequence as CSV. Each frame starts with a newly loaded animator so the
result is repeatable. `Test-FaceParity.ps1` runs the original x86 DLL twice
and demands byte-identical references, then compares every vertex coordinate
and row identity with the native x64 probe (default absolute tolerance 1e-4).
It also requires at least one actual moving vertex in the corpus.
The same reference run now records the sequencer's `AddMacroMuscle` and
`MultMacroMuscle` calls in `*-x86-muscles.csv`, repeats them, and checks that
the trace is byte-identical. `S2_FACE_MUSCLE_TRACE_PATH` enables this trace
for a standalone `FaceProbe` run. The `muscle` column is an ordinal assigned
on first observation within one probe process, not an asset ID or stable
muscle name; compare traces for the same input and call order. The wrapper
forwards each call to the real x86 animator, so the vertex oracle remains
unchanged. This intermediate oracle is intended to isolate native sequencer
errors from native bone/vertex-processing errors. It does not yet compare a
native x64 muscle trace.
An optional `S2_FACE_MUSCLE_MAP_PATH` writes a second CSV mapping each trace
ordinal to the original x86 macro-muscle object's inline name. This uses an
observed x86-only object layout (name at byte offset 8); it is diagnostic
evidence, not a native ABI contract. `Test-FaceParity.ps1` repeats and hashes
both CSVs and rejects any trace ordinal without a name. All six initial
head/sequence pairs and 20 representative sequences passed this named-oracle
check. The latter exercised 19 distinct names, including eye channels,
expressions and speech phonemes. The names make it possible to compare a
future native sequencer's semantic output independently of pointer identity.

Build `FaceProbe` in both `G:\SS\lab\build-x86` and `build-x64` with CMake.
Build x86 `Game` with this source tree. Prepare a fresh isolated LabRun, copy
that experimental `Game.exe`/PDB into it, and set `S2_FACE_FIXTURE_DIR` to an
existing folder in its `evidence` directory before `Start-LabRun.ps1` starts
the process. Loading a mission save exports `head-<record>-<part>.bin` and
`sequence-<record>-0.bin` from the real resource loaders. These files, the
original `tree.mma` and the proprietary DLL stay in `G:\SS\lab`, never Git.

Reference-only check:

```powershell
& .\diagnostics\Test-FaceParity.ps1 `
  -FixtureDirectory 'G:\SS\lab\runs\stage2-x86-face-fixtures-01\evidence\face-fixtures' `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86Probe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -ReferenceOnly
```

Remove `-ReferenceOnly` and add
`-X64Probe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe'` for the
required parity check. The initial corpus from `AI_CTRL` contains three heads
and two sequences (six pairs, 2514 rows each); all six x86 pairs are
deterministic and deform vertices. The strict x64 gate passes all six cases at
1e-4 tolerance (2,514 rows per case). The representative 20-sequence subset
for head 56 also passes with the same gate; use its fixture directory in the
command above to repeat that broader check.
For the current two sequences, the reference trace contains 1 and 13
macro-muscle calls respectively per probe run (for each of the three heads).

Coverage still needed before declaring native facial animation complete:
multiple sequence types (idle, speech, expression masks), overlapping tracks,
all head segments/topologies, saved/reloaded heads, transformable FaceGen GDP
and `.mmt` morphs, texture-weight user items, and a live x64 portrait/head
render compared to x86. The small corpus is an implementation guide, not a
substitute for these cases.

Corpus expansion: the x86 game's `facefixtures` harness command enumerates
the `HeadSeqs` database table and exports each accessible original sequence
stream into `S2_FACE_FIXTURE_DIR` (existing opt-in fixture directory). An
isolated run at `G:\SS\lab\runs\stage2-x86-face-corpus-01` exported 6,780
`MMSF` streams; the command is not part of normal gameplay. A representative
test subset in that run's `evidence\representative-fixtures` contains one
head and 20 sequences selected across file sizes, durations and header
counts, including the original two idle sequences. The reference-only
`Test-FaceParity.ps1` run passed repeatability for all 20; 13 changed vertices, and
macro-muscle traces ranged from 0 to 13 calls at the sampled frames. These
header counts are not yet a verified track taxonomy, and a zero-call trace
does not mean an empty sequence: it may act at unsampled times or through
other channels. The complete exported corpus and proprietary bytes stay in
the ignored lab directory. Use the representative subset for a wider red/green
x64 gate; additional named speech/expression cases still need classification.

Native `MMSF` envelope progress: `NativeSequenceData` reads the v1
32-byte header with checked payload length, duration and track count. The
x86 `ISequencer::SequenceTime` and `TracksCount` values matched those header
fields for five structurally different sampled files. The field at offset 20
is the size of the prelude after the 32-byte envelope: track records start at
`32 + preludeSize`. Each track has a 32-byte header, its second word gives the
following payload length, and the payload begins with a u32-length-prefixed
name. The decoder checks every boundary, name and final cursor, preserving
the eight header words and record offsets for later event parsing. It decoded
all tracks in all 6,780 exported sequences, with prelude sizes 76 (6,658
files), 84 (one file) and 100 (121 files). `NativeSequenceDataTests` passes on
both x86 and x64. Within a track, header word 2 is its event count and word 3
is the serialized name length. The native decoder now recognizes macro-event
records with a 32-byte header, length-prefixed name, sample count and two
equal-sized float arrays. It recognized 124,841 such events across the corpus;
10,856 other event-bearing tracks remain opaque (including sound formats).
The two arrays are evaluated as a nonuniform cubic-Hermite curve with endpoint
secants and centered interior slopes; the resulting expression is divided by
100. `NativeSequenceEvaluate` emits the active named events at requested times.
`Test-FaceParity.ps1 -NativeSequenceDecode ... -NativeSequenceEvaluate ...
-ReferenceOnly` compares the x86 DLL's call order, names and values against
this native sequencer evaluator. All 20 representative sequences passed at
five samples each (51 calls, maximum observed expression delta 1.2e-7).
An additional nine-time boundary sample set passed on the six initial pairs;
`S2_FACE_SAMPLE_TIMES` overrides the probe's default times for such checks.
For sequence 6008 on head 56, a separate 100-ms sweep over all 30,000 ms
matched 915 x86 calls to 915 native evaluations in the same order, with no
name/value mismatch at 1e-5 tolerance and maximum delta 1.19e-7.
This first established sampled sequencer-expression parity separately from
tree resolution and vertex deformation. The x64 API bridge now also uses this
evaluator in `ISequencer::RenderMacroMuscles`, as checked below. The later
strict vertex checks now pass on the bounded corpora described above.

The x86-only `S2_FACE_TREE_DUMP_PATH` probe option exports a deterministic
macro-operation graph after loading the original `tree.mma`. Two fresh dumps
were byte-identical. Its root is `AnimationLibrary` with eight direct
operations; the observed graph contains 222 unique macro nodes and 1,675
operations (1,519 nested-macro links and 156 effect links). The dump uses
private x86 vtable/object layouts and is diagnostic evidence, not a portable
API or a native implementation. The original v4 `MMLF` file is 146,740 bytes.
`NativeMMTreeData` now checks its envelope and recursively decodes all
length-delimited operation records. The serialized file contains exactly
1,675 operations; 221 records have children, corresponding to the x86 root's
221 descendant macro nodes. The native `--graph` output (root plus those
nodes, with paths and direct child counts) matches the x86 graph line for
line. `Test-MMTreeParity.ps1` repeats the x86 dump byte-for-byte and checks
the native graph and total operation count automatically. The record header's
first word classifies every operation: 1 maps to x86 macro operation type 4,
3 to type 0, and 4 to types 1–3. All 1,396 unnamed records are references:
header word 5 points backward to a named record of the same class. The native
decoder resolves and validates these links. The test now also compares all
1,675 operation classes and all 1,519 resolved macro-target names against
the x86 runtime in order; the observed run passed with no mismatch. This
does not yet distinguish x86 effect operation types 1, 2 and 3 natively.
`NativeMMTreeDataTests` covers nested records, malformed sizes, truncation
and pointer arguments on both architectures. This is **structural/name-
resolution** parity only: effect payloads and their mapping to head muscles
remain uninterpreted, so this does not establish deformation parity.

The x64 `IMMTree` bridge now owns native macro objects for all 222 named nodes;
`RootMacroMuscle` and exact-name `FindMacroMuscle` return stable pointers.
The x86 probe confirmed that the original lookup returns each of the 222
visited nodes for its name. The x64 sequencer now evaluates decoded macro
events and calls the animator's `AddMacroMuscle` with the resolved tree object.
`Test-FaceParity.ps1 -CallsOnly` compares these **actual x64 API calls** with
the x86 calls by time, operation, name, order and value, while deliberately
not treating static x64 vertices as a success. It passed all six initial
head/sequence cases and all 20 representative sequence cases, with maximum
observed value difference 1.2e-7 at 1e-5 tolerance. This only covers the
sampled event types and times; other operation modes still need coverage.
This call-level gate does not establish effect or vertex parity.

```powershell
& .\diagnostics\Test-FaceParity.ps1 `
  -FixtureDirectory 'G:\SS\lab\runs\stage2-x86-face-corpus-01\evidence\representative-fixtures' `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86Probe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -X64Probe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe' `
  -CallsOnly
```

```powershell
& .\diagnostics\Test-MMTreeParity.ps1 `
  -TreeFile 'G:\SS\lab\baseline\tree.mma' `
  -HeadFile 'G:\SS\lab\runs\stage2-x86-face-fixtures-01\evidence\face-fixtures\head-56-0.bin' `
  -SequenceFile 'G:\SS\lab\runs\stage2-x86-face-fixtures-01\evidence\face-fixtures\sequence-6008-0.bin' `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86Probe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -NativeDecoder 'G:\SS\lab\build-x64\RelWithDebInfo\NativeMMTreeDecode.exe' `
  -NativeApiCheck 'G:\SS\lab\build-x64\RelWithDebInfo\NativeMMTreeApiCheck.exe'
```

`MMLF` operation records encode a sampled curve: serialized type `n` is the
sample count (3–10 observed), and the curve occupies `16 + 8n` bytes before
the optional name. The same nonuniform cubic-Hermite evaluator used by `MMSF`
matches isolated x86 effects. A direct `AddMacroMuscle` call enters a macro's
children with the original expression; it does **not** apply the macro
definition's stored curve first. Traversed child operation curves are applied
before following references or emitting a leaf effect. `NativeMMTreeMacroEvaluate`
implements this traversal. For the direct `(base)_NOSE_L` and nonlinear
`(base)_LIP_up_midle` cases, isolated x86 muscle amplitudes match the native
leaf outputs over several positive and negative expressions to about 1e-7.
The x86 raw muscle snapshot includes address-dependent bytes, so these tests
repeat and compare only its observed semantic float fields.

The x64 `IAnimator::AddMacroMuscle` now accumulates class-3 muscle effects;
`ComputePhysics` ignores an accumulator only when its absolute-value
denominator is below 1e-4, then computes each muscle's new endpoint as
`A + (1 - amplitude) × (B - A)`.
`Test-MMTreeMacroEffects.ps1` compares all 36 resulting amplitudes with the
original x86 runtime. For `##BLINK`, 11 leaf effects target the available
head muscles and ten muscles move at expression 0.5; all three sampled heads
pass with maximum amplitude delta 9.7e-8. The genuine sequence-6008 frame at
time 15000 also passes `Test-FaceMuscleStateParity.ps1` on all three heads,
with ten active muscles and maximum delta 3.54e-8; time 22500 passed on head
56 with two active muscles.

For class-3 muscle effects, x64 `Process` first builds a displacement vector
`Dᵢ = max(componentAᵢ, 0) × componentBᵢ × (newBᵢ - originalBᵢ)` per influence.
This rule matched all 20 vertices influenced by an isolated nostril muscle;
negative `componentA` was confirmed to contribute no motion in the isolated
lip case. Multiple moving muscles are **not** added independently. For `n>1`
influences the original x86 `Process` stores the coefficient
`cₙ = -ln(n)/(2(n-1))` (confirmed on all 287 explicit vertices of each of
three heads, maximum delta 2.8e-17). It scales each `Dᵢ` by
`exp(cₙ × Σⱼ≠ᵢ |dot(Dᵢ/|Dᵢ|, Dⱼ) × componentBⱼ| /
(componentBᵢ × |Dᵢ|))`, skipping contributions whose denominator is below
1e-4. Attached bones rotate
the displacement vector separately from the vertex position; only the latter
receives bone translation. This formula predicts x86 vertex 239 of the
overlapping cheek/eyelid macro to about 2e-7, and the native bridge passes
strict 419-vertex comparisons for `(base)_lEYELID_DOWN_R` and `##BLINK`
on all three sampled heads at expression 0.5 (maximum delta 1.91e-6).
`##BLINK` also passes on head 56 at -0.5, 0.25 and 0.8. The former
`Distrust` mismatch is resolved: the x86 effect callback accumulates
`Σ sign(vᵢ) vᵢ²` and `Σ |vᵢ|` separately, and `ComputePhysics` divides the
first by the second when the denominator reaches 1e-4. Directly summing
effect values was incorrect for shared graphs. The x64 bridge now uses this
rule for muscle and bone effects. At expression -0.226, `Distrust` matches
all 36 muscle amplitudes and all 419 vertices on each of the three heads;
the maximum vertex delta on head 56 is 2.86e-6. The original six-case
sequence gate and the 20-sequence representative gate both pass. They do
not establish complete animation across all assets and runtime paths.

`S2_FACE_VERTEX_STATE_PATH` and `S2_FACE_STATE_SNAPSHOT_TIME` make the x86
probe export the original per-vertex interaction coefficients into a CSV in
the ignored lab directory. `NativeMMTreeFilterEffects` writes a derived tree
there with all but one leaf target of a selected macro zeroed; it never
changes the original `tree.mma`. This gave independent x86 references for
`a_EyelidLOW_UW_R` and `a_Cheek_FW_R` before validating their combination.

The x86 `ComputePhysics` bone snapshot identifies runtime operation type 2
as local-Y rotation (amplitude at byte 528) and type 1 as local-Z rotation
(amplitude at byte 524). The type is stored in the referenced definition's
payload, not reliably in the reference's header word: two definitions of
`a_Eye_ROT_L` share a name but select different axes. The rotations run
between the serialized B and A
matrices. Local-Y uses minus the effect value as angle; local-Z uses the value.
For multiple influences, each bone-transformed or unchanged vertex position
is blended by its `componentB`, normalized by the sum of positive weights.
When all weights are zero, x86 uses equal weights across the influences;
this was confirmed by jaw vertex 127 on heads 86 and 102. Isolated macros
for both upper eyelids, both eyes, and jaw now match the x86 reference on all
three sampled heads, with maximum vertex difference 2.39e-6. `EyesYaw`
also passes as a two-bone effect. `Test-FaceBoneEffectParity.ps1` repeats the
x86 run, checks the two opaque amplitude fields, requires actual vertex
motion, and enforces x64 vertex parity. `HeadPitch` changes bone state but
no head vertices in this probe, so it is not counted as geometric parity.
The combined sequence gate now passes on the bounded corpora above.

```powershell
& .\diagnostics\Test-MMTreeMacroEffects.ps1 `
  -HeadFile 'G:\SS\lab\runs\stage2-x86-face-fixtures-01\evidence\face-fixtures\head-56-0.bin' `
  -SequenceFile 'G:\SS\lab\runs\stage2-x86-face-fixtures-01\evidence\face-fixtures\sequence-6008-0.bin' `
  -TreeFile 'G:\SS\lab\baseline\tree.mma' -GameRoot 'G:\SS\lab\baseline' `
  -X86Probe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -X64Probe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe' `
  -NativeHeadDecode 'G:\SS\lab\build-x64\RelWithDebInfo\NativeHeadDecode.exe' `
  -NativeMacroEvaluate 'G:\SS\lab\build-x64\RelWithDebInfo\NativeMMTreeMacroEvaluate.exe' `
  -MacroName '(base)_NOSE_L' -Expression 0.5 -RequireVertexParity
```

Replace `-MacroName` with `##BLINK` and keep `-RequireVertexParity` to run the
passing combined muscle-and-bone regression. The genuine sequence gate above
must remain separate until all macro accumulation semantics match x86.

`Test-FaceMuscleStateParity.ps1` takes the same head, sequence, tree, game
root and probe paths plus `-Time 15000`; unlike the forced-macro test, it
compares muscle state after the real sequence has rendered that frame.
`Test-FaceBoneEffectParity.ps1` takes the same paths plus a bone-only macro
name (for example `Eyelid_Up_Bone_L.1`, `Eye_Bone_L.1`, `Jaw_Bone.1` or
`EyesYaw`) and uses the native head and macro evaluators.

An isolated x64 game run at `G:\SS\lab\runs\stage2-x64-face-native-load-01`
loaded the existing `AI_CTRL` save to `LOAD-SLOT-DONE` and exited through the
harness `quit` command without a recorded crash. Its screenshot
`evidence\loaded-game.png` proves only that the scene rendered; it does not
establish that the minimap portrait or live facial motion is correct. This
run preceded the native effect work; the later check is described below.

A fresh x64 archive from commit `feb52c8` and isolated run
`G:\SS\lab\runs\stage2-face-live-feb52c8-01` loaded the same `AI_CTRL` save
to `LOAD-SLOT-DONE`. After selecting hero 1 through DirectInput scan code
`0x02`, `evidence\portrait-visible.png` shows her rendered face in the lower
left portrait; `evidence\inventory-full-physical.png` also shows the head on
the full-body model. Two captures two seconds apart (`portrait-frame-a.png`
and `portrait-frame-b.png`) show a changed portrait pose. This establishes
visible live output for one selected hero, not x86/x64 pixel parity or
coverage of every portrait, dialogue and FaceGen path. The first screenshots
were misleadingly cropped: `Capture-Window.ps1` was not DPI-aware and on the
125% desktop lost the right and bottom 20% of the physical game window.
The capture helper now uses physical-pixel window bounds.
During that test run, the user also watched the game directly and reported
that the portrait was present and facial animation looked correct. This is
useful qualitative live validation, independent of the screenshot captures,
but does not expand the automated x86/x64 parity corpus.

The remaining x64 `ITransformer` bridge is still a stub. This does not affect
the static head and portrait checked above, but transformable
FaceGen heads use `Res\FaceGenHead.gdp` and `.mmt` to generate a morphed
animator in `CHeadTransformInfo::Recalc`; its x86 oracle exists, but the native
transformer implementation and live check are still needed. The serialized tree also contains
runtime type-3 bone effects (for example `Head_shake` and tongue rotation).
The sampled static heads have no vertex influences attached to their head or
neck bones, so the passing vertex corpus does not exercise these effects.

`FaceGDPProbe` is an x86 oracle for the previously untested FaceGen path. Build
it from the x86 CMake tree and run it with `FaceGenHead.gdp`,
`FaceGenHead.mmt`, an output prefix, and optionally a macro name/amplitude.
It extracts the GDP's default animator and saves both the transformer's
generated animator and its processed vertex CSV. For example, with the
original DLLs first on `PATH`:

```powershell
& 'G:\SS\lab\build-x86\RelWithDebInfo\FaceGDPProbe.exe' `
  'G:\SS\lab\baseline\Res\FaceGenHead.gdp' `
  'G:\SS\lab\baseline\Res\FaceGenHead.mmt' `
  'G:\SS\lab\runs\stage2-face-gdp-oracle-01\evidence\nose' Nose 0.5
```

The original GDP has one transformable object (`800`) and 419 vertices. Its
default animator is 31,612 bytes; the neutral and `Nose=0.5` generated
animators are 21,970 bytes each. The Nose case changes 48 vertices, with
maximum component delta 0.26445627 from neutral. Reloading both generated
animators through the original x86 `FaceProbe` reproduced all 419 vertices
exactly, confirming the oracle output is usable. The x64 `FaceProbe` matches
the GDP's *default* animator within 1.91e-6. The x64 decoder now also loads
the original DLL's 0x37D30DC0 `Save` format, including variable-length
muscle and bone records and compact vertex tables. `Test-FaceGDPParity.ps1`
recreates neutral and `Nose=0.5` x86 generated streams, checks that x86 can
reload them unchanged, and compares their 419 processed vertices to x64. Both
cases pass at 1e-4 tolerance. Passing `-SequenceFile` adds an animated x86/x64
comparison. With `sequence-6008-0.bin`, all 2,514 animated vertex rows now
pass at 1e-4, with maximum component delta 2.38e-6. The earlier 690-row
failure (maximum delta 0.1866033) exposed the omitted saved-stream weight;
the native decoder now reconstructs it. The native transformer path itself
remains a stub, so generating a morph without the x86-produced saved stream
is not yet supported. Oracle artifacts are in the ignored
`G:\SS\lab\runs\stage2-face-gdp-oracle-01\evidence` directory.

The x64 `IGDPFile`/`IGDPObject` now read the original OLE compound GDP with
Windows structured storage, without pre-extracting assets. The transformer's
read surface (`ITransformerInput::Size/Get`), default animator, object and
subobject names, transformable flag, vertex count, and Morph-derived data list
are native. `GDPStorageProbe` inventories the storage hierarchy.
`NativeGDPApiCheck` compared all 50 data-list items byte-for-byte with the
original x86 `IGDPObject::Get` dumps; the default animator bytes and
subobject count (seven) also matched. The remaining material/UV/triangulation methods of
`IGDPObject` are placeholders, and `ITransformer::Load/Generate` is still
unimplemented. The x64 `Game.exe` links successfully with the new GDP reader.

FaceGen's `FaceGenHead.mmt` is a second MMLF v4 graph variant with a class-2
root, rather than the main `tree.mma` class-1 root. The native decoder now
accepts both while retaining the stricter effect-type validation for the
ordinary facial tree. It decodes 158 FaceGen records and the native `IMMTree`
resolves 30 macro nodes. The GDP's `*_M.mld` streams use the already decoded
0xAD5A018D head format; for example `EuroM_M.mld` has 129 morph muscles,
419 explicit vertices and six bones, whereas its `*_A.mld` animation stream
has 36 muscles, 419 vertices and seven bones. `Morph.txt` defines the
archetype parameter combinations; `Links.dat` appears to carry channel links
and still needs decoding.
`Test-FaceGDPTransformParity.ps1` runs the same GDP and MMT through the x86
and x64 `FaceGDPProbe` executables, comparing neutral, positive/negative
`Nose`, `Age` and `Gender` generated vertices directly. The x86 reference
shows `Age` and `Gender` each alter all 419 vertices at both extremes;
`Nose=0.5` alters 48. The test currently fails at x64 `ITransformer::Load` even
for neutral output; unlike `Test-FaceGDPParity.ps1`, it does not pass an
x86-precomputed animator to the native side. This is the implementation gate
for native generation. An optional x86 `S2_FACE_TRACE_TRANSFORMER=1` probe
prints the original transformer's vtable and post-`Load` worker dispatch
addresses. In the examined DLL, public `Generate` (RVA `0x13580`) forwards to
an internal worker dispatch at RVA `0x12300`. In this GDP, its runtime counts
are 36 animation muscles, 7 bones and 129 morph muscles. The worker checks
its 129-record link table against the morph-muscle count and invokes several
distinct generation passes
(`0x118e0`, `0x11a60`, `0x11bb0`, `0x120e0`, `0x12140`, `0x11a90`,
`0x12260`). This narrows the remaining reverse-engineering target; the
near-linear archetype fit below is not a substitute for these passes.
With `S2_FACE_DUMP_WORKER_PREFIX=<path>`, the x86 probe also exports the
worker's post-`Generate` scalar, item and vector arrays for comparison. On
the reference GDP, `Nose=0.5` leaves all 36 muscle scalars and 79 output
vectors byte-identical to neutral, while `Age=1` and `Gender=1` change both
arrays. Thus a Nose morph can alter vertices without changing the animation
muscle anchors; Age and Gender also reshape those anchors. These snapshots
are exploratory internal evidence, not native parity results.

The x64 `IAnimator::Save` now round-trips a loaded original-format or
saved-format stream byte-for-byte. `Test-FaceGDPParity.ps1` checks that
round-trip on the x86-generated FaceGen animators before comparing vertices.
This preserves an already generated stream; it does not synthesize a new one.
An exploratory least-squares fit of the neutral x86 output to the 16 unique
archetype `*_A.mld` positions suggests near-linear blending of adult male,
adult female and older African/Asian/Arab forms. Its maximum raw-position
residual is about 5.8e-4, so this is a clue, not an exact native recipe.

Further x86 runtime inspection identified what the compact format omits.
With `S2_FACE_VERTEX_INFLUENCES_PATH` and
`S2_FACE_STATE_SNAPSHOT_TIME=0`, `FaceProbe` exports each loaded vertex's
runtime `(muscle, componentA, componentB)` after physics. The original
`head-56-0.bin` and its x86-saved/reloaded copy produced byte-identical CSVs
for all 1,083 influences. The saved file contains only `componentA`; the
original DLL reconstructs `componentB` at load time. For example, vertex 0's
first influence has A=0.53200537 and restored B=0.363512456. The saved
FaceGen `Nose=0.5` stream has the same 1,083 runtime influences but different
weights (for that influence A=0.53536272, B=0.337188423). A is the
unclamped projection parameter of the vertex on the muscle's anchor segment:
all 1,083 source-stream values match this calculation within 2.21e-7.
B is a spatial falloff: clamp A to `[0,1]`, measure distance from the vertex
to that nearest point on the muscle's anchor segment, and evaluate the
muscle's five-point nonuniform cubic Hermite curve at that distance. Clamp
the result to `[0,1]`; beyond the last knot, B is zero. The x64 decoder
recovers the five X knots and Y values from each saved muscle record and now
matches all 1,083 runtime B values in both the saved static and the FaceGen
`Nose=0.5` heads within 2.98e-7. The reference CSVs are in the ignored
oracle run's `evidence` directory.
`Test-FaceGDPParity.ps1 -SequenceFile <sequence-6008-0.bin>
-NativeHeadDecode <NativeHeadDecode.exe>` includes a direct 1,083-row
x86/native influence-weight gate in addition to the vertex comparisons;
on the current FaceGen case its maximum coefficient delta is 2.24e-7.

Format investigation: `FaceProbe animator.bin neutral.csv saved.bin` asks the
original x86 `IAnimator::Save` for its canonical serialized form. For
`head-56-0.bin`, the source stream is 31,612 bytes and the saved form is
21,970 bytes. Reloading that saved form through the original DLL produced
**byte-identical neutral and animated CSV** for sequence 6005. This gives the
native parser a second representation to validate against, but the shipping
x64 engine must load the original resource stream without invoking the x86 DLL
or requiring preconverted copyrighted assets.

Native decoder progress: `lifestudio_native_data` reads the original
`0xAD5A018D` header and both bounds-checked vertex tables directly on x64,
without the DLL. `NativeHeadDecode` parsed all three current head streams:
36 muscles, 7 bones, 419 total vertices, 287 explicitly stored and 132 stored at the
tail as implicit records. Their raw coordinates were compared with the x86
neutral `Process` output. All 132 implicit positions matched at the printed
precision; the maximum differences over the full heads were 0.00906, 0.00938
and 0.00875 for heads 56, 86 and 102. Only the last 22 explicit vertices of
each head exceeded 1e-4; the other source positions matched at that tolerance.
Skipping `ComputePhysics` and `FillUnused` in the x86 probe did not remove
those differences, so the remaining correction is in `Process` or its inputs.
Copying positions is still not a parity implementation.
`NativeHeadDataTests` covers a synthetic valid stream and truncation/invalid
count/index cases under CTest. The decoder now also extracts each original
172-byte **muscle** record's type, null-terminated name, and two
three-dimensional anchor points, and rejects nonfinite anchors or influences
outside the muscle table. All three real heads have the same 36 named muscles,
7 separate bones, and 1,083 valid muscle influences. The 22 formerly divergent neutral-pose
discrepancies belong to vertices influenced by the two `_aEye_*` muscles.
This classification was verified against the original x86 API:
`MusclesCount()=36`, `BonesCount()=7`, `VerticesCount()=419`. An optional
`S2_FACE_BONE_SNAPSHOT_PREFIX` probe snapshot confirmed seven opaque runtime
bone objects; the source stream's trailing 1,192-byte region contains seven
separate bone records. The native decoder now parses their fixed-length names,
two 4×3 matrices and bounded lists of attached muscle indices; e.g., the jaw
bone references muscles `4,5,16,18,19,20,17`. `NativeHeadDecode --bones`
exposes the decoded matrices for diagnostics. The earlier
description of the 36 fixed-size records as bones was incorrect and has been
corrected throughout the native data model.
For each eye, the x86 neutral result matches the row-vector composition
`matrixB × matrixA` stored in its bone record. The native bridge now applies
that data-driven composition to vertices with one influence attached to a
bone; it does not hard-code eye indices or fitted coefficients. The separate
`Test-FaceNeutralParity.ps1` gate compares all 419 neutral vertices on each
of the three heads against a repeated x86 oracle. It passes at 1e-4 tolerance,
with maximum observed differences 1.91e-6, 9.6e-7 and 1.43e-6. This neutral
check alone does not establish animated parity. Animated parity is now
verified on the bounded six-case and 20-case corpora above. A later live x64
portrait check, described above, confirmed visible motion in one scene and
the user independently reported no visual issue. FaceGen and additional
head/sequence variants remain unverified.
`Test-NativeHeadDecode.ps1` automates the three-head raw-coordinate comparison
with the x86 neutral oracle. Its loose explicit-vertex tolerance measures the
known gap rather than accepting it as finished animation; the full
`Test-FaceParity.ps1` remains the strict red/green gate.
