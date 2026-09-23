# LifeStudio x86 → native x64 parity

Status: the x86 reference is executable and deterministic; the current x64
`lifestudio_x64_stub.cpp` fails the same probe. **No native parity claim yet.**

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
deterministic and deform vertices. The current x64 stub fails before comparison
because it cannot load the muscle tree or sequence. This is the expected red
test, not a passing result.
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

Native `MMSF` envelope progress: `NativeSequenceData` now reads the v1
32-byte header with checked payload length, duration and track count. The
x86 `ISequencer::SequenceTime` and `TracksCount` values matched those header
fields for five structurally different sampled files. The x64 decoder parsed
all 6,780 exported sequences; `NativeSequenceDataTests` passes on both x86
and x64. The fourth header field (offset 20) has unknown semantics and is
preserved without interpretation. No track/keyframe or muscle-tree decoding
is claimed, and the strict x64 vertex parity test remains red.

The partial x64 API bridge now wires the native original-head and `MMSF`
envelope decoders into `IAnimator::Load` and `ISequencer::Load`. It accepts
the v4 `MMLF` envelope for the game's `tree.mma` and emits raw head positions
through `IAnimator::Process`; the muscle tree, keyframe evaluation, bone
physics and exact final vertex transform are **not** implemented. As a result,
`FaceProbe` now proceeds through load/process on x64, rather than failing at
load. The strict six-case gate still fails numerically: 778–952 coordinate
components per case exceed 1e-4, with maximum delta 0.34–0.58. This is an
intermediate native processing milestone, not working facial animation.
An isolated x64 game run at `G:\SS\lab\runs\stage2-x64-face-native-load-01`
loaded the existing `AI_CTRL` save to `LOAD-SLOT-DONE` and exited through the
harness `quit` command without a recorded crash. Its screenshot
`evidence\loaded-game.png` proves only that the scene rendered; it does not
establish that the minimap portrait or live facial motion is correct.

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
7 separate bones, and 1,083 valid muscle influences. The 22 neutral-pose
discrepancies belong to vertices influenced by the two `_aEye_*` muscles.
This classification was verified against the original x86 API:
`MusclesCount()=36`, `BonesCount()=7`, `VerticesCount()=419`. An optional
`S2_FACE_BONE_SNAPSHOT_PREFIX` probe snapshot confirmed seven opaque runtime
bone objects; the source stream's trailing 1,192-byte region contains seven
separate bone records, still awaiting a checked native decoder. The earlier
description of the 36 fixed-size records as bones was incorrect and has been
corrected throughout the native data model.
For each eye, the x86 output is consistent
with one near-identity affine YZ transform (fit residual below 4e-7 on all
three heads); that is diagnostic evidence, **not** a hard-coded correction or
a proven general animation formula. `IAnimator::Process` in the x86 DLL calls
per-vertex influence evaluation before filling unused vertices, so the next native
step is reproducing those muscle/bone calculations from source data. Neither this
decoder nor the test currently satisfies the full x64 parity gate. Muscle
behavior, the trailing bone section, sequence evaluation and FaceGen remain
to be implemented natively.
`Test-NativeHeadDecode.ps1` automates the three-head raw-coordinate comparison
with the x86 neutral oracle. Its loose explicit-vertex tolerance measures the
known gap rather than accepting it as finished animation; the full
`Test-FaceParity.ps1` remains the strict red/green gate.
