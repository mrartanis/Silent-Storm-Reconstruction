# LifeStudio x86 → native x64 parity

Status: the x86 reference is executable and deterministic; the x64 bridge
loads and processes neutral heads but fails animated vertex parity.
**No native animation parity claim yet.**

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

Reference-only check (does **not** claim x64 success):

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
deterministic and deform vertices. The current x64 bridge decodes the tree,
evaluates macro events and applies sampled muscle effects to vertices, but
only the verified upper-eyelid bone channel is animated, and full parity
remains incomplete; the strict gate is red.
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
evaluator in `ISequencer::RenderMacroMuscles`, as checked below. The strict
x64 vertex parity test remains red.

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
`ComputePhysics` applies the observed `abs(amplitude) < 1e-4` deadband and
computes each muscle's new endpoint as `A + (1 - amplitude) × (B - A)`.
`Test-MMTreeMacroEffects.ps1` compares all 36 resulting amplitudes with the
original x86 runtime. For `##BLINK`, 11 leaf effects target the available
head muscles and ten muscles move at expression 0.5; all three sampled heads
pass with maximum amplitude delta 9.7e-8. The genuine sequence-6008 frame at
time 15000 also passes `Test-FaceMuscleStateParity.ps1` on all three heads,
with ten active muscles and maximum delta 3.54e-8; time 22500 passed on head
56 with two active muscles.

For class-3 muscle effects, x64 `Process` now moves an explicitly weighted
vertex by `max(componentA, 0) × componentB × (newB - originalB)` per influence.
This rule matched all 20 vertices influenced by an isolated nostril muscle;
negative `componentA` was confirmed to contribute no motion in the isolated
lip case. Both direct macros pass a strict 419-vertex comparison on head 56,
with no component over 1e-4 and maximum delta 1.91e-6. That is a scoped
geometric result, **not** complete facial animation: the forced `##BLINK`
case has 94 mismatched components (maximum 0.00179) after the upper-eyelid
bone transform. Other class-4 channels and simultaneous muscle influences
remain incomplete. With only the verified eyelid channel enabled, the full
six-case strict gate is still red: 291–719 components per case exceed 1e-4,
with maximum delta 0.11–0.31. The wider experimental rule that interpreted
every class-4 channel as local-Y rotation was rejected: it changed these
numbers but was unsupported by the x86 bone-state evidence.

For the isolated `Eyelid_Up_Bone_L.1` and `Eyelid_Up_Bone_R.1` macros,
the original x86 `ComputePhysics` rotates the corresponding bone around local
Y by minus the class-4 effect value, between the serialized B and A matrices.
For multiple influences, each bone-transformed or unchanged vertex position
is blended by its `componentB`, normalized by the sum of those weights. The
effect value and all 419 vertex positions now match the x86 reference for both
eyelids on all three sampled heads (maximum vertex difference 2.39e-6).
`Test-FaceBoneEffectParity.ps1` repeats the x86 run, checks the opaque bone's
observed amplitude field at byte 528, and enforces x64 vertex parity.
This does not establish correctness for eye, jaw, neck, or head rotations.

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

`Test-FaceMuscleStateParity.ps1` takes the same head, sequence, tree, game
root and probe paths plus `-Time 15000`; unlike the forced-macro test, it
compares muscle state after the real sequence has rendered that frame.
`Test-FaceBoneEffectParity.ps1` takes the same paths plus
`-MacroName 'Eyelid_Up_Bone_L.1'` (or the right-side name) and uses the
native head and macro evaluators. It requires a bone-only macro.

An isolated x64 game run at `G:\SS\lab\runs\stage2-x64-face-native-load-01`
loaded the existing `AI_CTRL` save to `LOAD-SLOT-DONE` and exited through the
harness `quit` command without a recorded crash. Its screenshot
`evidence\loaded-game.png` proves only that the scene rendered; it does not
establish that the minimap portrait or live facial motion is correct. This
run preceded the native effect work and has not been repeated with the current
bridge.

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
with maximum observed differences 1.91e-6, 9.6e-7 and 1.43e-6. This **does not**
establish animated parity: only one class-4 channel is verified; other bone
channels, full multi-influence interactions and FaceGen remain to be
implemented natively, and the strict
`Test-FaceParity.ps1` gate remains red.
`Test-NativeHeadDecode.ps1` automates the three-head raw-coordinate comparison
with the x86 neutral oracle. Its loose explicit-vertex tolerance measures the
known gap rather than accepting it as finished animation; the full
`Test-FaceParity.ps1` remains the strict red/green gate.
