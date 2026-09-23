# LifeStudio x86 → native x64 parity

Status: the x86 reference is executable and deterministic. The native x64
bridge passes strict animated vertex parity on the initial 3-head × 2-sequence
set, the 20-sequence representative set for head 56, all 136 exported game
head streams with sequences 6008 and 6005, and the focused two-segment
head-11 neck-motion test. A DB-backed custom head at three game-control
presets also passes ordinary save-slot reload, a 554-sequence animated-vertex
sample and a layered head/speech/expression gate on both architectures.
The editor-default saved head additionally passes all 6,780 DB-exported
game sequences at five sampled times each.
Current x86 and x64 builds
render that loaded custom head in the live portrait. This is bounded corpus
and runtime evidence, **not** a claim of x86/x64 rendered-pixel parity or
every possible FaceGen slider/sequence combination.
The staged x64 `Game.exe` dependency table (`dumpbin /dependents`, checked
2026-09-23) contains no `LifeStudioHeadAPI.dll` or `GDPFile.dll`; x64 links
the native implementation through `lifestudio_init`. The original DLLs are
used only by the x86 oracle, not as a runtime fallback for the x64 game.

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
errors from native bone/vertex-processing errors. The current gate also
compares the native x64 trace by muscle name, operation, time and value;
both traces must contain finite values.
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

Coverage limits: the probes compare deterministic LifeStudio calls, saved
heads and CPU textures, not pixel-identical GPU frames. The paired live
screenshots show a rendered custom portrait on x86 and x64 but are not
time-synchronized. The eight game expression masks and speech overlaps have
dense sampled coverage below; the game's scene-level event scheduler is not
independently captured frame by frame. All 6,780 exported sequences have
five-time coverage on one saved FaceGen head, not continuous-time or every
slider combination.

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
the ignored lab directory. `New-FaceSequenceStratifiedCorpus.ps1` selects a
second, reproducible 31-sequence set spanning all three prelude sizes,
three- and five-track files, long/large files and high track counts. The
strict `Test-FaceParity.ps1` x86/x64 call-and-vertex gate passed all 31 cases
on head 56 (2,514 vertex rows each, zero mismatches at 1e-4). The selection
includes actual `Phonemes/A` and `Phonemes/B` speech tracks. Sampled frames
are still just a subset of each sequence's duration.

```powershell
& .\diagnostics\New-FaceSequenceStratifiedCorpus.ps1 `
  -FixtureDirectory 'G:\SS\lab\runs\stage2-x86-face-corpus-01\evidence\face-fixtures' `
  -HeadFile 'G:\SS\lab\runs\stage2-x86-face-corpus-01\evidence\representative-fixtures\head-56-0.bin' `
  -OutputDirectory 'G:\SS\lab\runs\face-stratified-corpus-01'
& .\diagnostics\Test-FaceParity.ps1 `
  -FixtureDirectory 'G:\SS\lab\runs\face-stratified-corpus-01' `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86Probe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -X64Probe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe' `
  -OutputDirectory 'G:\SS\lab\runs\face-stratified-parity-01'
```

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
Dense 100-ms x86/x64 sweeps also passed on two speech formats: sequence 371
(100 frames, 68 macro calls, 42,319 vertex rows) and sequence 1 (84 frames,
97 calls, 35,615 rows). At sampled frames, sequence 371 had up to four
simultaneous calls, so the test covers overlapping phonemes. Both strict
vertex gates had zero mismatches at 1e-4.

The game also stacks several sequences on one animator before computing
physics (for example speech, idle and expression masks). `FaceProbe` can
exercise that path with `S2_FACE_OVERLAY_SEQUENCE_FILE` and a signed
`S2_FACE_OVERLAY_TIME_SHIFT` in milliseconds: it renders the primary and
overlay into the same animator, then computes physics once. The
`Test-FaceOverlayParity.ps1` gate runs both a primary-only baseline and an
overlaid case, compares x86/x64 call traces and every vertex, and requires
that the overlay adds calls and changes vertices. On three real heads with
speech sequence 371 and idle sequence 6008, 100 frames at 100-ms intervals
and a -1500-ms overlay shift passed: each head had 68 primary calls versus
288 overlaid calls, 42,319 vertex rows per run and zero x86/x64 mismatches at
1e-4. This verifies a bounded game-like overlap, not every timing
combination. The fixture directory has the three heads and the primary
sequence; proprietary streams stay outside the repository.

```powershell
& .\diagnostics\Test-FaceOverlayParity.ps1 `
  -FixtureDirectory 'G:\SS\lab\runs\face-overlay-three-heads-corpus-01' `
  -OverlaySequence 'G:\SS\lab\runs\stage2-x86-face-corpus-01\evidence\face-fixtures\sequence-6008-0.bin' `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86Probe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -X64Probe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe' `
  -OutputDirectory 'G:\SS\lab\runs\face-overlay-parity'
```

The game's `FaceExpression2Sequences` table maps eight non-neutral emotion
masks to sequence IDs: Smile 6010, Anger 7548, Worry 7550, Fear 7551, Sad
7552, Happy 7553, Smirk 7554 and Disgust 7555. The `faceexpressions` harness
command resolves these IDs through the game's own `GetSequenceByExpression`;
Calm maps to 7547 but has no macro events, and the reserved Rage enum has no
separate row. `Test-FaceExpressionMaskParity.ps1` overlays each real mask on
speech sequence 371, sampling 0..900 ms in 100-ms steps on three real heads.
All eight masks pass the strict x86/x64 call-and-vertex gate (4,609 vertex
rows per head and mask, zero mismatches at 1e-4); the wrapper also requires
each mask to add macro calls and visibly alter the computed vertices.

```powershell
& .\diagnostics\Test-FaceExpressionMaskParity.ps1 `
  -FixtureDirectory 'G:\SS\lab\runs\face-overlay-three-heads-corpus-01' `
  -SequenceDirectory 'G:\SS\lab\runs\stage2-x86-face-corpus-01\evidence\face-fixtures' `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86Probe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -X64Probe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe' `
  -OutputDirectory 'G:\SS\lab\runs\face-expression-masks'
```

Denser 10-ms sweeps of Anger and Sad also passed on all three heads for
0..1000 ms: 101 frames, 42,738 vertex rows and zero mismatches per head and
mask. The Anger mask exposed one vertex at 500 ms crossing an influence's 1e-4
activation boundary because x64 calculated its muscle endpoint as
`A + (1-a)*(B-A)` while x86 uses `B - a*(B-A)`; the operation order is now
matched. The Sad/CRY mask exposed a disabled nested `Cry1` branch: its first
serialized payload word is 1, so x86 skips it within CRY, although a direct
call to Cry1 still evaluates its children. The native MMT walker now makes
that distinction. The other observed nonzero flag (2) also occurs on live
bone-related operations and is not skipped. The 31-sequence stratified corpus
still passes after both corrections.

The game can keep an idle active underneath speech and its expression mask;
after the short mask ends, `CHeadAnimator` renders its duration-minus-two
tail frame while speech continues. `FaceProbe` now accepts a second overlay
through `S2_FACE_OVERLAY2_SEQUENCE_FILE`, its signed
`S2_FACE_OVERLAY2_TIME_SHIFT`, and optional `S2_FACE_OVERLAY2_HOLD_LAST`.
`Test-FaceThreeWayParity.ps1` runs idle sequence 6008, speech sequence 371
starting at 1500 ms, and a DB-backed emotion mask starting at the same time,
all on one animator before one physics solve. It compares both idle+speech
and idle+speech+mask against x86, and checks that the mask still contributes
calls after its nominal end. Sad, Anger and Worry passed on three heads over
100 frames at 100-ms intervals: 42,319 vertex rows per head and case, zero
x86/x64 mismatches at 1e-4. For Sad and Anger, each head had 320 two-layer
calls versus 405 three-layer calls; Worry had 490 three-layer calls. Run a
representative case with:

```powershell
& .\diagnostics\Test-FaceThreeWayParity.ps1 `
  -FixtureDirectory 'G:\SS\lab\runs\face-threeway-idle-corpus-01' `
  -SpeechSequence 'G:\SS\lab\runs\stage2-x86-face-corpus-01\evidence\face-fixtures\sequence-371-0.bin' `
  -ExpressionSequence 'G:\SS\lab\runs\stage2-x86-face-corpus-01\evidence\face-fixtures\sequence-7552-0.bin' `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86Probe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -X64Probe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe' `
  -OutputDirectory 'G:\SS\lab\runs\face-threeway-sad-parity'
```

These bounded tests do not cover every possible head, speech, idle and mask
combination or a live rendered game session.

The `headfixtures` game-harness command exports every `Heads` resource stream
referenced by `NDb::CHead` into `S2_FACE_FIXTURE_DIR`. The baseline DB yielded
136 streams: 134 first segments and two second segments. Of these, 113 use
the original `0xAD5A018D` format and 23 use the compact `0x37D30DC0` saved
format. Head 11 is the special two-segment case: its main saved stream has a
976-byte `Neck_Zone/Upper/Lower` appendix, and its second stream has only
implicit vertices (section marker 1). The native decoder now accepts both
variants, validates all eight zone masks and upper/lower landmark pairs, and
preserves the original bytes for `IAnimator::Save`. Its head
bone is the parent transform for the 15 neck-influenced vertices in the main
segment. The complete 136-stream corpus passed x86/x64 neutral+idle parity
with sequence 6008: zero mismatches at 1e-4, including both head-11 segments.
To repeat, export the streams with `headfixtures` in an x86 harness run, put
the idle sequence beside them in a lab-only fixture directory, then run
`Test-FaceParity.ps1` against that directory.

Sequence 6005 has real `Head_LR` and `Head_shake` events, and overlapping
`Distrust` facial motion. `Test-FaceNeckMotionParity.ps1` now passes both
head-11 segments across 9,500..16,000 ms: 6,936 vertex rows, zero mismatches
at `1e-4`. `Test-FaceNeckMacrosParity.ps1` also passes 32 forced values from
-1 to +1 over all 416 main-segment vertices: `HeadPitch`, `HeadYaw`,
`Head_nod`, `Head_LR`, `Head_shake`, `NECK_UD`, `NECK_LR` and `NECK_ROTATE`
(maximum delta `2.86e-6`). The saved neck attachment counter-rotates the
head's local-Y/Z channels, and the neck bone itself uses those opposite Y/Z
signs; the runtime type-3 local-X effect follows the negative effect value.
`Test-FaceNeckAnchorsParity.ps1` separately locks the
eight lower control landmarks across 11 frames (88 rows, maximum delta
`1.43e-6`). The original DLL's `IAnimator::Process` also applies a neck-zone
pass: each masked vertex retains barycentric weights against two adjacent
upper/lower control lines and a center line between head/neck bones. The x64
bridge now performs the same pass using weights from the neutral pose, so
facial movement is not double-applied. The passing full comparison is outside
Git at `G:\SS\lab\runs\face-head11-neck-zone-pass-02`. Repeating sequence
6005 over all 136 exported game head streams also passed with zero mismatches
at `G:\SS\lab\runs\face-all-heads-motion-parity-01`.
`Test-FaceNeckLayeredParity.ps1` adds speech and a held expression mask over
the sequence's simultaneous head-shake/Distrust interval; both head-11
segments passed (4,046 rows, 29 macro calls per segment, zero vertex
mismatches at `1e-4`). This specifically guards against double-applying
facial motion while deforming the neck zones.

```powershell
& .\diagnostics\Test-FaceNeckMotionParity.ps1 `
  -FixtureDirectory 'G:\SS\lab\runs\face-head11-neck-corpus-01' `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86Probe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -X64Probe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe' `
  -OutputDirectory 'G:\SS\lab\runs\face-head11-neck-zone-pass-02'
```

The sequence-1 sweep first exposed one dropped `AA` phoneme at 8,100 ms:
the x86 DLL produced 97 calls but the native evaluator only 96. A full
`NativeSequenceDecode --curve-audit` of the 6,780 exported files found 64
nonincreasing terminal knots among 124,841 macro events. They have authored
shapes such as `{0,1,.9999,1}`, `{0,1,1}` and `{0,1.08,1}`; the x86 DLL
evaluates the active first segment despite the final knot. The native
sequence evaluator now does that specifically for these terminal forms,
while the shared curve validator stays strict elsewhere. The sequence-1
sweep now passes with all 97 calls. `Test-FaceTerminalCurveParity.ps1`
audits the complete exported corpus, samples each anomalous event at its
start, quarter, midpoint, three-quarter and final active tick, then compares
x86 and x64 macro calls and vertex coordinates. All 64 anomalies across 63
sequences pass; every target event is present in the sampled x86 trace.
This covers the overshoot and exact-duplicate forms as well as the small
terminal reversal, without claiming full-time parity for every sequence.

```powershell
& .\diagnostics\Test-FaceTerminalCurveParity.ps1 `
  -FixtureDirectory 'G:\SS\lab\runs\stage2-x86-face-corpus-01\evidence\face-fixtures' `
  -HeadFile 'G:\SS\lab\runs\stage2-x86-face-corpus-01\evidence\representative-fixtures\head-56-0.bin' `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86Probe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -X64Probe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe' `
  -NativeSequenceDecode 'G:\SS\lab\build-x64\RelWithDebInfo\NativeSequenceDecode.exe' `
  -NativeSequenceEvaluate 'G:\SS\lab\build-x64\RelWithDebInfo\NativeSequenceEvaluate.exe' `
  -OutputDirectory 'G:\SS\lab\runs\face-terminal-curve-parity-strict-01'
```
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
the x86 runtime in order; the observed run passed with no mismatch. At this
earlier structural stage, effect operation types 1, 2 and 3 were not yet
distinguished; the later runtime/vertex gates below supersede that limit for
game-used effects. `NativeMMTreeDataTests` covers nested records, malformed
sizes, truncation and pointer arguments on both architectures. The structural
check alone does not establish deformation parity.

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

After the head-11 neck-zone work, a second isolated x64 run at
`G:\SS\lab\runs\face-live-final-01` loaded `AI_CTRL` to `LOAD-SLOT-DONE`
with the latest native bridge. `LabInput.exe` selected hero 1 by DirectInput
scan code `0x02`; `evidence\portrait-a.png` and `portrait-b.png` capture the
visible portrait two seconds apart. In the fixed portrait ROI
(`x=10..104`, `y=625..739`), 6,659 of 10,925 RGB pixels changed (absolute
channel-difference sum 461,355). The harness `quit` command exited the
process cleanly. This confirms visible live portrait motion on this build,
but does not isolate which facial channel changed or compare rendered pixels
against the x86 game.

The x64 mesh `ITransformer` now generates and serializes transformable
FaceGen heads from `Res\FaceGenHead.gdp` and `.mmt`, as checked by the direct
parity gate below. The live in-game editor preview and separate
texture-weight `UserItem` channels now pass x86/x64 gates below. The serialized tree also contains
runtime type-3 bone effects (for example `Head_shake` and tongue rotation).
The native bridge evaluates this as a local-X rotation; the head-11 motion
gate above verifies the native neck-zone path together with this channel.

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
the native decoder now reconstructs it. The direct native transformer now
generates its own saved stream without an x86-produced animator. Oracle artifacts are in the ignored
`G:\SS\lab\runs\stage2-face-gdp-oracle-01\evidence` directory.

The x64 `IGDPFile`/`IGDPObject` now read the original OLE compound GDP with
Windows structured storage, without pre-extracting assets. The transformer's
read surface (`ITransformerInput::Size/Get`), default animator, object and
subobject names, transformable flag, vertex count, and Morph-derived data list
are native. `GDPStorageProbe` inventories the storage hierarchy.
`NativeGDPApiCheck` compared all 50 data-list items byte-for-byte with the
original x86 `IGDPObject::Get` dumps; the default animator bytes and
subobject count (seven) also matched. The remaining material/UV/triangulation methods of
`IGDPObject` are placeholders; the game-used mesh `ITransformer::Load/Generate`
and collected texture `UserItem` output are implemented. The
x64 `Game.exe` links successfully with the GDP reader and transformer.

FaceGen's `FaceGenHead.mmt` is a second MMLF v4 graph variant with a class-2
root, rather than the main `tree.mma` class-1 root. The native decoder now
accepts both while retaining the stricter effect-type validation for the
ordinary facial tree. It decodes 158 FaceGen records and the native `IMMTree`
resolves 30 macro nodes. The GDP's `*_M.mld` streams use the already decoded
0xAD5A018D head format; for example `EuroM_M.mld` has 129 morph muscles,
419 explicit vertices and six bones, whereas its `*_A.mld` animation stream
has 36 muscles, 419 vertices and seven bones. `Morph.txt` defines the
archetype parameter combinations. `Links.dat` consists of a 12-byte header,
129 fixed 64-byte morph-channel names, 43 fixed 64-byte output-channel names,
and a 129×43 byte matrix. The reference GDP's entire matrix is zero, so the
semantics of nonzero entries remain unverified.
`NativeFaceGenData` now parses the five `Morph.txt` coordinates and both
`HEAD` and `COMB` rules, resolves each referenced archetype, and decodes its
animation and morph streams through the native head decoder. It validates
the `Links.dat` signature, exact size, both name tables and channel counts,
preserving the raw matrix bytes. It does not yet interpret nonzero matrix
entries. On the real GDP,
`NativeFaceGenApiCheck FaceGenHead.gdp` reports 24 `HEAD` rules, 24 `COMB`
rules, 16 unique archetypes, 419 vertices, 36 animation muscles, 129 morph
muscles, 43 output channels and zero nonzero matrix entries.
`NativeFaceGenDataTests` exercises rule syntax and percentage handling under
both x86 and x64 CTest. The loader is now connected to the x64 mesh
transformer. `Test-FaceGDPTransformParity.ps1` runs the same GDP and MMT
through x86 and x64 `FaceGDPProbe`, comparing 11 direct native generation
cases: neutral, both signs of Nose, Age, Gender and Nationality, plus two
editor slider combinations including the default. All 419 generated vertices
pass at 1e-4 tolerance, with maximum observed delta 8.1e-6. With both
`FaceProbe` executables and `-SequenceFile`, it also reloads each native
stream under x86 and x64 and compares 2,514 animated rows per case; all 11
pass, with maximum observed delta 8.11e-6. Neither test uses x86-precomputed
intermediate heads on the native side. The x86 reference shows `Age` and
`Gender` each alter all 419 vertices at both extremes; `Nose=0.5` alters 48.
Run the direct mesh and saved-animation gate with:

```powershell
& .\diagnostics\Test-FaceGDPTransformParity.ps1 `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86GDPProbe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceGDPProbe.exe' `
  -X64GDPProbe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceGDPProbe.exe' `
  -X86FaceProbe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -X64FaceProbe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe' `
  -SequenceFile 'G:\SS\lab\runs\stage2-x86-face-fixtures-01\evidence\face-fixtures\sequence-6008-0.bin' `
  -OutputDirectory 'G:\SS\lab\runs\face-direct-native-gate'
```

An optional x86 `S2_FACE_TRACE_TRANSFORMER=1` probe
prints the original transformer's vtable and post-`Load` worker dispatch
addresses. In the examined DLL, public `Generate` (RVA `0x13580`) forwards to
an internal worker dispatch at RVA `0x12300`. In this GDP, its runtime counts
are 36 animation muscles, 7 bones and 129 morph muscles. The worker checks
its 129-record link table against the morph-muscle count and invokes several
distinct generation passes
(`0x118e0`, `0x11a60`, `0x11bb0`, `0x120e0`, `0x12140`, `0x11a90`,
`0x12260`). This narrows the remaining reverse-engineering target; the
near-linear archetype fit below is not a substitute for these passes.
The original transformer's `+0x10` and `+0x14` fields are respectively a
36-muscle animation base and a 129-muscle morph head. `Generate` first calls
`Process` on the morph head, then passes its 419 positions to the worker
alongside the animation base and output animator. The x86-only
`S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX` probe exports both canonical input
streams and the morph head's live processed vertices before that transfer.
For neutral and `Nose=0.5`, the two saved morph-head streams are byte-identical:
the Nose change resides in live muscle state, not serialized source positions.
`Test-FaceGDPMorphStageParity.ps1` extracts those x86 intermediate heads,
replays `Nose=0.5` through x86 and native x64 `IAnimator`, and compares all
2,514 neutral/animated rows against the original intermediate `Process`
output. It passes with 48 moving vertices, zero x86 replay delta and maximum
x64 delta 6.92e-6 at 1e-4 tolerance. The Nose-induced vertex change in the
final generated animator matches the intermediate morph-head change within
1.9e-6. Thus native morph deformation is boundedly verified after receiving
an x86-generated morph head; producing that head and transferring it into a
new animator remain separate unfinished native transformer work. Run with:

```powershell
& .\diagnostics\Test-FaceGDPMorphStageParity.ps1 `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86GDPProbe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceGDPProbe.exe' `
  -X86FaceProbe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -X64FaceProbe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe' `
  -SequenceFile 'G:\SS\lab\runs\stage2-x86-face-fixtures-01\evidence\face-fixtures\sequence-6008-0.bin' `
  -OutputDirectory 'G:\SS\lab\runs\stage2-face-gdp-oracle-01\morph-stage-gate'
```

The selector embedded in the original x86 transformer holds 16 floating-point
archetype weights after `ComputePhysics`. With
`S2_FACE_SELECTOR_WEIGHTS_PATH=<weights.csv>`, `FaceGDPProbe` exports them in
GDP archetype order. Normalizing those weights and blending each archetype's
`*_M.mld` source vertex coordinates and both muscle anchor points reproduces
the transformer's saved 129-muscle morph head. `BlendFaceGenGeometry` performs
this blend natively; `Test-FaceGenBlendParity.ps1` obtains fresh x86 weights
and intermediate heads, then checks native x64 geometry for neutral, both
signs of Age and Gender, both signs of Nose and Nationality, and two
advanced-editor slider combinations. All eleven cases pass at
1e-4: 419 vertices and 129 pairs of muscle anchors per case, with maximum
observed vertex delta 1.91e-6 and anchor delta 3.82e-6. Run with:

```powershell
& .\diagnostics\Test-FaceGenBlendParity.ps1 `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86GDPProbe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceGDPProbe.exe' `
  -X64BlendCheck 'G:\SS\lab\build-x64\RelWithDebInfo\NativeFaceGenBlendCheck.exe' `
  -X64SelectorParamCheck 'G:\SS\lab\build-x64\RelWithDebInfo\NativeFaceGenSelectorParamCheck.exe' `
  -X64GameWeightCheck 'G:\SS\lab\build-x64\RelWithDebInfo\NativeFaceGenGameWeightCheck.exe' `
  -X64OutputTransferCheck 'G:\SS\lab\build-x64\RelWithDebInfo\NativeFaceGenOutputTransferCheck.exe' `
  -OutputDirectory 'G:\SS\lab\runs\face-blend-gate'
```

The blend subtests deliberately feed x86-selected weights to the native
blender; the optional game-weight subtest separately checks native selection.
The same gate also compares the complete decoded 36-muscle animation base and
129-muscle morph rig on all eleven cases, including the editor default. The
checks cover vertices, muscle anchors and curves, influence projections,
bone transforms and topology. The optional output-transfer check composes a
final head from freshly captured x86 intermediates and compares its decoded
fields with x86 `Generate`. These staged checks do not prove native end-to-end
`ITransformer::Generate` by themselves; the direct gate above does for the
tested mesh cases. For example, neutral x86
weights begin `0.05, 0.05, 0.025, 0`
for Euro M/W/O/C and `6.75, 6.75, 3.375, 0` for African M/W/O/C. `Nose=0.5`
does not change them; `Age` and `Gender` do. The game-used weight selection
is reproduced as described below.

Scope of the native port is the **game's calls**, not every LifeStudio
FaceGen feature. `Main/iAdvFaceGen.cpp::UpdateHead` sets 14 named sliders,
with discrete positions 0..100 mapped to `[-1,+1]`; `Main/LSHead.cpp`
passes recognized macro names to the mesh and texture transformers, asks
both to `ComputePhysics/Generate`, saves the mesh animator into a committed
head, and reads texture `UserItem/UserValue` channels from the separate
texture rig. The editor defaults to Age=0, Gender=0, Nationality=-1,
Lips/Chin/Nose/Brows/Cheeks=0, hair controls=+1, and color/damage/glasses
controls=-1. `EyeGlasses` is not a macro in this MMT; the game handles its
model swap separately. In a one-slider x86 probe, only Age, Gender and
Nationality changed the 16 archetype weights; the remaining recognized
sliders leave those weights unchanged but may still affect morph geometry or
texture channels. A no-macro probe is **not** equivalent to the editor's
explicit `Nationality=0` state. `S2_FACE_SELECTOR_PARAMS_PATH=<path>` exports
the transformer's five selector inputs for diagnostics; the default is five
zeros, while `Nationality=1` sets the three ethnicity inputs to approximately
`100,-58.44155,-55.84415`. The original selector's general-purpose
interpolation is supplied by the x86-only `TriangLib.dll`; the native port
implements only the game's discrete slider path. The target is the game's
editor and committed-head behavior, not a general-purpose FaceGen API clone;
the bounded commit/save/animation checks are documented below.
The native MMT evaluator now accepts the game's class-5 user-item leaves,
allowing it to evaluate Age/Gender/Nationality macros without rejecting their
texture effects. `EvaluateGameFaceGenParameters` maps the resulting effects
to the five Morph.txt selector inputs. The optional selector-parameter check
in the command above compares those five x64 inputs directly with freshly
exported x86 values on all eleven cases; its maximum observed delta is
3.82e-6.

The 16 game archetype weights factor into four nationality-group totals and
the same Age/Gender proportions within each group. On the observed game
range, `adult=(100-Age)/125`, `older=1-adult`, and
`male=(100+Gender)/200`, where Age and Gender are the evaluated selector
parameters, not raw slider amplitudes. The native MMT evaluator clamps a
child expression to its knot endpoints as the x86 macro tree does; this was
needed for intermediate Nationality positions. The four nationality totals
for the editor's 101 integer positions (0..100) are captured as a small
oracle-derived coefficient table in `NativeFaceGenGameEthnicity.inc`.
`SelectGameFaceGenWeights` uses that table and the native Age/Gender
calculation, with no x86 DLL at runtime. This is deliberately not a general
5D TriangLib replacement. Off-grid nationality amplitudes are linearly
interpolated but have not been parity-qualified. Direct x86/x64 weight gates
pass all 101 Nationality, 101 Age and 101 Gender positions, 27 combinations,
and the 11-case blend corpus (including the editor default), with maximum
raw-weight delta 1.91e-6 at 1e-4 tolerance. Run the full weight sweep with:

```powershell
& .\diagnostics\Test-FaceGenGameWeightFactorization.ps1 `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86GDPProbe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceGDPProbe.exe' `
  -X64WeightCheck 'G:\SS\lab\build-x64\RelWithDebInfo\NativeFaceGenGameWeightCheck.exe' `
  -OutputDirectory 'G:\SS\lab\runs\face-native-weights-gate' `
  -NativeEthnicity
```

The animation-base geometry discrepancy is resolved. Disassembly of the x86
selector's blend loop (RVA `0x10830`) showed that both generated heads use
the selector's per-archetype coordinate arrays, while their muscle sources
differ. `S2_FACE_SELECTOR_COORDS_PATH=<csv>` dumps those arrays. On EuroM,
their 419 positions match the `*_M.mld` source positions; 64 positions differ
from `*_A.mld`, by as much as 0.6854. The correct 36-muscle animation base
therefore blends `*_M.mld` vertex coordinates with `*_A.mld` muscle anchors.
`BlendFaceGenAnimationGeometry` now does this. A further x86 field comparison
established that muscle curves are weighted averages (maximum delta 1.67e-5),
explicit vertex influence membership is unchanged and `componentA` is the
vertex's projection onto the newly blended muscle segment (maximum delta
2.60e-7). For each bone, x86 retains the first archetype's orientation,
averages only its translation, and recomputes the inverse matrix. That rule
matches the editor-default reference within 9.54e-7. The native
`BlendFaceGenAnimationHead` implements these rules and reconstructs the
runtime falloff component. The 11-case `--animation-fields` gate now checks
the complete decoded base head, with maximum observed curve, projection,
falloff and bone deltas of 1.53e-5, 5.07e-7, 8.35e-7 and 9.54e-7 on the
editor default. The compact `IAnimator::Save` stream can now also be encoded
natively: encoding an x86 saved stream round-trips all 21,970 bytes exactly,
and encoding an original `EuroM_A.mld` stream reloads through the original
x86 DLL with zero neutral-vertex delta. The x64 mesh transformer uses this
encoder for its generated output. A bounded committed-head slot and motion
gate is documented below; the live editor preview has a separate gate below.

The native `BlendFaceGenMorphHead` now builds the complete decoded 129-muscle
morph rig by the same rule, and its 11-case field gate passes at 1e-4. The
output worker's head transfer is bounded too. For the reference cases, x86
`Generate` leaves the saved 36-muscle and seven-bone sections byte-identical
to its animation base; only explicit and implicit vertex records change. Its
final source position is `animation-base position + (morph Process position -
morph-base source position)` at the nine-digit probe precision, including
Age, Gender, Nose and the editor default. Explicit vertex influence
coefficients are recomputed against the unchanged animation muscles.
`ComposeFaceGenOutputHead` implements this transfer. The 11-case
`NativeFaceGenOutputTransferCheck` compares all decoded final fields against
fresh x86 output; the tested vertex deltas are zero, and the editor-default
influence deltas are at most 3.43e-7. This check uses x86-precomputed
intermediate heads and morph `Process` positions to isolate the transfer.
The direct x64 `ITransformer` now performs native morph processing, output
composition and serialization; the staged transfer check remains a diagnostic
that isolates that operation. The texture channels and live editor preview
have separate gates below.

`UserItem` status: **game-used path implemented**, not an editor-only API.
It is needed for a game-visible result, although it is not queried
each render frame. On committing a customized hero, `CreateHeadInfo` reads
the texture rig's named channels and bakes their face/eye/eyelash layer
weights into `CHeadInfo::pTexture`; the world renderer then passes that
persisted texture to the head material. Without these channels, a unique
face would lose its selected texture layers after leaving the editor. The
original x86 DLL stores those channels on the collecting `ITransformer`,
not its output `IAnimator`; the x64 bridge implements this game-used path
on the transformer. `AnimatorStub::UserItem/UserValuesCount/UserValue` remain
unimplemented because no game call reads user items from a standalone mesh
animator. Only class-5 MMT leaves reached by active game macros are
collected, with one clamped value per leaf and multiple values when names
repeat. Unknown names return `-1`. The uncollected mesh rig retains its
21,970-byte mesh-only save stream. `Test-FaceGDPUserItemParity.ps1` compares
x86 and x64 channel presence, counts and values (ignoring internal numeric
IDs) for neutral, single-slider, editor-default and five positions of each
of the eight game macro controls that emit user items. All 47 cases pass;
the maximum tested value delta is `1.34e-7` at `1e-4` tolerance. The editor
default has positive `Eyes_01`, `Hair_10` and `European.0` weights, so the
channel path is not merely returning absent/zero values. The 47-case gate
was rerun on 2026-09-23 and passed. Run with:

```powershell
& .\diagnostics\Test-FaceGDPUserItemParity.ps1 `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86GDPProbe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceGDPProbe.exe' `
  -X64GDPProbe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceGDPProbe.exe' `
  -OutputDirectory 'G:\SS\lab\runs\face-user-parity-01'
```

`Test-FaceGenEditorPreviewParity.ps1` exercises the real Advanced FaceGen
interface in both game builds. It loads an isolated mission, opens the
editor, sets actual `CScroll` controls through harness commands, runs
`UpdateHead`, then bakes the editor's current preview through
`CreateLSHeadInfo`. On 2026-09-23 the default CPU texture hash matched
exactly (`d5a9b10a85e06e2e`), Nose=100 changed the mesh on both builds,
and EyesColor=100 changed the texture to the same hash on both
(`54dcd612902b3674`). Each preview contained a static mesh and nonempty
texture. The mesh stream hashes differ between architectures; the separate
committed-head pose gates compare rendered geometry. This editor gate does
not establish GPU pixel parity or validate mouse input; the latter is
unreliable in the game harness. Run with separate staged game copies:

```powershell
& .\diagnostics\Test-FaceGenEditorPreviewParity.ps1 `
  -X86GameRoot 'G:\SS\lab\runs\facegen-bake-harness-01-x86\game' `
  -X64GameRoot 'G:\SS\lab\runs\face-live-final-01\game'
```

The `facegenbake` game-harness command exercises more than the LifeStudio
API: it chooses a transformable `CComplexHead` from `game.db`, applies three
fixed slider presets, calls the same `CHeadTransformInfo::CreateHeadInfo`
commit path as the editor, and hashes the resulting persisted 256×256 CPU
texture. `Test-FaceGenBakeParity.ps1` launches separate staged x86/x64 game
directories, sends the harness command atomically, requires a static mesh and
a committed nonempty texture in every case, and compares the complete pixel
hash exactly. Each case also serializes its `CHeadInfo` with the game's
`CStructureSaver`, reloads it, and verifies the 21,970-byte animator stream
and all 65,536 CPU pixels byte-for-byte. All three cases passed on head record 94: hashes
`e61211fd62cc6efc`, `d3155898178747fc`, and `72eb80435311b3f3`.
These distinct hashes show that slider changes reach the bake. The test
proves the DB-backed texture/mesh commit and object-serialization path, not
GPU display. A separate ordinary save-slot reload test is described below.
Given separately staged game directories with the
matching x86 and x64 executables, run:

```powershell
& .\diagnostics\Test-FaceGenBakeParity.ps1 `
  -X86GameRoot 'G:\SS\lab\runs\facegen-bake-harness-01-x86\game' `
  -X64GameRoot 'G:\SS\lab\runs\facegen-bake-harness-01-x64\game'
```

`Test-FaceGenSlotParity.ps1` goes through an ordinary game slot, not just a
detached `CHeadInfo` round-trip. Its harness extension takes the first merc
from a loaded `AI_CTRL` mission, commits a game-DB-backed transformable head
using real slider controls, saves to a fresh named slot with `CICSave`, loads
that slot with `CICLoad`, and inspects the merc's restored `CHeadInfo`.
The script checks head identity, static-head flag, mesh animator-stream length
and byte hash, texture presence, nonzero pixel count and exact CPU texture hash before and
after reload, then compares x86 with x64. The paired run on 2026-09-23 passed:
head 94, 21,970 animator bytes, 28,672 nonzero texture pixels and hash
`e4e884b9a57e9b6c` on both architectures before and after slot reload.
Each architecture's animator bytes also survived its own reload exactly:
x86 hash `c0137ab666e743c2`, x64 hash `18b3f185a066e3b9`.
The stream bytes are **not** cross-architecture identical, so raw byte
equality is not a valid pose-parity gate. The paired pose test below uses
each architecture's committed stream as its own input.
The output slot was `FACEGEN_PARITY_01` in two isolated lab copies; no user
save was overwritten. This proves the game-used persisted FaceGen data path,
but not x86/x64 GPU pixel parity or actual animation of that custom head in
the rendered mission. Repeat with freshly staged x86/x64 game directories
containing the same `AI_CTRL` source slot and current `Game.exe` builds:

```powershell
& .\diagnostics\Test-FaceGenSlotParity.ps1 `
  -X86GameRoot 'G:\SS\lab\runs\facegen-bake-harness-01-x86\game' `
  -X64GameRoot 'G:\SS\lab\runs\face-live-final-01\game' `
  -OutputSlot 'FACEGEN_PARITY_04'
```

The slot test also exports each reloaded animator to a lab-only `.bin` file.
`Test-FaceGenCommittedMotionParity.ps1` runs the original x86 DLL on the x86
stream and the native bridge on the x64 stream, with the same game sequence
and muscle tree. It repeats the x86 reference for determinism, requires
actual moving vertices, and compares each sampled vertex coordinate. On the
head-94 output from slot `FACEGEN_PARITY_03`, sequence 6005 at seven active
times passed: 3,352 vertex rows, 352 moving rows, maximum x86/x64 delta
`7.15e-6` at `1e-4` tolerance. This is direct animation parity for one
committed custom head and one game sequence, not every editor preset or
sequence and not GPU pixel parity.

```powershell
& .\diagnostics\Test-FaceGenCommittedMotionParity.ps1 `
  -X86Stream 'G:\SS\lab\runs\facegen-bake-harness-01-x86\game\_facegen_FACEGEN_PARITY_03.bin' `
  -X64Stream 'G:\SS\lab\runs\face-live-final-01\game\_facegen_FACEGEN_PARITY_03.bin' `
  -SequenceFile 'G:\SS\lab\runs\face-head11-neck-corpus-01\sequence-6005-0.bin' `
  -TreeFile 'G:\SS\lab\baseline\tree.mma' `
  -GameRoot 'G:\SS\lab\baseline' `
  -X86Probe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -X64Probe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe' `
  -OutputDirectory 'G:\SS\lab\runs\facegen-committed-motion-gate-01'
```

The same pair of reloaded custom heads also passed the 31-sequence
stratified game corpus with each sequence's default five sample times:
77,934 paired vertex rows, 16,898 moving rows and maximum delta
`6.68e-6`. The corpus includes speech phonemes and expression tracks, but
these are sampled times, not an exhaustive all-sequence/all-frame proof.
Run the corpus gate with `Test-FaceGenCommittedCorpusParity.ps1` and the
same stream/probe/tree paths, plus
`-SequenceDirectory 'G:\SS\lab\runs\face-stratified-corpus-01'` and a
new lab-only `-OutputDirectory`.

`Test-FaceGenCommittedLayeredParity.ps1` adds simultaneous head motion
(sequence 6005), speech (371) and a held expression (7552) on this same
reloaded custom head. The focused six-frame gate passed 2,933 vertex rows,
1,688 moving rows, with maximum delta `7.15e-6` at `1e-4` tolerance.
The gate now accepts `-SampleTimes` for denser timing checks without changing
the sequence or overlay setup. On 2026-09-23, 167 sampled times per preset
spanned 13,999..17,999 ms at 25-ms intervals plus start/end boundary probes
around the speech/expression overlays. All three saved presets passed
70,392 paired vertex rows each, with 47,061 / 46,479 / 46,893 moving rows;
maximum deltas were `7.16e-6` / `7.40e-6` / `7.63e-6`. This crosses the
expression's 15,102-ms end, including the held-last state, but does not
prove continuous-time parity. For example, construct the sample list with:

```powershell
$times = @(13999,14000,14099,14100,14101,15099,15100,15101,17998,17999) +
         @(14000..17999 | Where-Object { ($_ - 14000) % 25 -eq 0 })
$sampleTimes = (@($times | Sort-Object -Unique) -join ',')
# Pass -SampleTimes $sampleTimes to Test-FaceGenCommittedLayeredParity.ps1
```

`Test-FaceGenCommittedExpressionCorpus.ps1` applies that same 167-time
head-motion/speech/held-expression grid to **all eight** game DB emotion
masks on the separately saved x86/x64 editor-default head. All eight passed
on 2026-09-23: 563,136 paired vertex rows, 357,198 moving rows, and a
maximum delta of `7.63e-6` at `1e-4`. The grid includes times before each
expression starts and after its duration, so the held-last transition is
exercised. This tests the LifeStudio overlay path, not the game's UI event
scheduler. Run with the same committed streams and sequence fixture paths
used by the corpus gate, plus a fresh `-OutputDirectory`.

The slot harness now accepts `facegencommit <case>` for three presets, each
explicitly setting the same 14 named controls that `iAdvFaceGen::UpdateHead`
passes to the live head. Case 0 is a mixed-control fixture, case 1 matches
the editor's default slider positions, and case 2 combines Age/Gender,
Nationality, five face-shape sliders and texture controls near their extremes.
`Test-FaceGenSlotParity.ps1 -CaseIndex <0..2>` passed a separate ordinary
slot for each case on x86 and x64. Every case preserved its own full animator
byte hash and texture hash across reload; x86/x64 texture hashes match
exactly and differ between cases:

| Case | Texture hash (both) | 31-sequence max vertex delta |
| --- | --- | ---: |
| 0 | `88c9fb604ab89759` | `6.68e-6` |
| 1 | `d5a9b10a85e06e2e` | `7.39e-6` |
| 2 | `55b6d5c8ac661569` | `7.39e-6` |

For each saved case, `Test-FaceGenCommittedCorpusParity.ps1` passed 31
stratified game sequences (77,934 paired vertex rows per case); the layered
head/speech/expression gate also passed all three (2,933 rows per case).
For broader coverage, `New-FaceSequenceHashSample.ps1` selected every
exported game sequence whose SHA-256 filename hash is 0 modulo 13, plus all
31 stratified sequences: 554 of the 6,780 DB-exported streams. The full
`Test-FaceGenCommittedCorpusParity.ps1` gate passed this 554-sequence sample
on all three isolated saved presets: 1,392,756 paired vertex rows per head.
The mixed, editor-default and extreme-shape cases had 357,095, 353,130 and
357,542 moving rows, with maximum deltas `6.92e-6`, `7.39e-6` and
`7.39e-6` respectively at `1e-4`. Results remain under
`G:\SS\lab\runs\facegen-mixed-554-gate-01`,
`facegen-editor-default-554-gate-01` and
`facegen-extreme-554-gate-01`. This is a reproducible 8.2% sample at each
sequence's default five frame times, not all 6,780 sequences or continuous
time coverage.
An additional complete-corpus run on the saved editor-default head (case 1)
passed **all 6,780 exported game sequences** on 2026-09-23: 17,044,920
paired vertex rows, 4,395,500 moving rows, maximum coordinate delta
`7.39000000038459e-6` at `1e-4` tolerance. It used each architecture's
separately saved animator stream and repeated every x86 reference run.
The ignored result directory
`G:\SS\lab\runs\facegen-editor-default-full-6780-gate-01` contains exactly
6,780 sequence directories and 20,340 CSVs (three per sequence), with no
incomplete directory or nonfinite coordinate token. This covers every
DB-exported sequence at its default five times, **not** every millisecond
or every slider combination. The other two saved presets retain the
554-sequence sampled coverage described above.
Use a fresh output directory for the selector; it refuses to overwrite an
existing fixture set.

```powershell
& .\diagnostics\New-FaceSequenceHashSample.ps1 `
  -SourceDirectory 'G:\SS\lab\runs\stage2-x86-face-corpus-01\evidence\face-fixtures' `
  -AlwaysIncludeDirectory 'G:\SS\lab\runs\face-stratified-corpus-01' `
  -OutputDirectory 'G:\SS\lab\runs\facegen-hash-sample-13-0' `
  -Modulo 13 -Residue 0
& .\diagnostics\Test-FaceGenCommittedCorpusParity.ps1 `
  -X86Stream 'G:\SS\lab\runs\facegen-bake-harness-01-x86\game\_facegen_FACEGEN_ISOLATED_1.bin' `
  -X64Stream 'G:\SS\lab\runs\face-live-final-01\game\_facegen_FACEGEN_ISOLATED_1.bin' `
  -SequenceDirectory 'G:\SS\lab\runs\facegen-hash-sample-13-0' `
  -TreeFile 'G:\SS\lab\baseline\tree.mma' -GameRoot 'G:\SS\lab\baseline' `
  -X86Probe 'G:\SS\lab\build-x86\RelWithDebInfo\FaceProbe.exe' `
  -X64Probe 'G:\SS\lab\build-x64\RelWithDebInfo\FaceProbe.exe' `
  -OutputDirectory 'G:\SS\lab\runs\facegen-editor-default-554-gate-01'
```
These tests verify game-used mesh and texture effects of the UI control
values. The slot gate additionally compares the per-unit race, hair and
four world/interface mesh record IDs before/after reload and across x86/x64.
It exposed a real persistence bug: `Nationality` changed only the shared
`CComplexHead::pBodyColor`, leaving the serializable per-unit
`CHeadInfo::pBodyColor` null. `CreateHeadInfo` now copies the chosen race into
the committed head, and `ChooseBodyColor` reads that per-unit field with a
fallback for older/stock heads. The three presets now persist race IDs
`3`, `1`, `3` on both architectures. Case 2 also persists hair model `4190`
and glasses model `4221` (the other two cases intentionally have neither).
The corrected case-2 texture hash and its 31-sequence and layered animation
gates all pass. An isolated x64 load of the `FACEGEN_HAIR_2` slot visibly
shows the committed hair and glasses on the portrait at
`G:\SS\lab\runs\face-live-final-01\evidence\facegen-hair-glasses-loaded.png`;
the process exited by harness `quit`. This is a live-render sanity check,
not synchronized x86/x64 pixel parity. The interactive editor's GPU preview
remains unverified.

The slot fixture now restores the shared `CComplexHead` race, hair and mesh
pointers immediately after installing the per-unit head, so later save/load
cannot succeed by accidentally reading those mutated DB-template pointers.
All three `FACEGEN_ISOLATED_<case>` slots passed the same x86/x64
model/texture/stream checks with template restoration. The case-2 x64
portrait still shows the selected hair
and glasses at
`G:\SS\lab\runs\face-live-final-01\evidence\facegen-isolated-hair-glasses-loaded-b.png`.
The current x86 build also loaded that isolated slot and showed the same
hair/glasses selection in
`G:\SS\lab\runs\facegen-bake-harness-01-x86\evidence\facegen-isolated-hair-glasses-loaded.png`.
These are separate, unsynchronized frames; no pixel-equality claim follows.

The five game-used face-shape controls were then tested individually with
`Test-FaceGDPTransformParity.ps1 -MacroName <name> -Amplitude 0.5` and its
optional `-X86FaceProbe/-X64FaceProbe/-SequenceFile` animated-output gate.
All five 11-case runs passed direct x86/x64 generated-vertex parity and x86
and x64 reload/animation parity at `1e-4`. None is a trivial no-op in the
x86 reference: the maximum neutral-to-positive vertex displacement is
`0.3454` for Lips, `0.4131` for Chin, `0.2645` for Nose, `0.3848` for Brows
and `0.3629` for Cheeks. Outputs are under
`G:\SS\lab\runs\facegen-shape-<name>-parity-01`.

The `FACEGEN_PARITY_03` slots were also loaded visibly in separate x86 and
x64 game windows (not through computer-use automation). `LabInput.exe`
selected the first merc; both runs showed the custom male face in the live
portrait, and each portrait changed between two captures. The x64 captures
are `G:\SS\lab\runs\face-live-final-01\evidence\facegen-reloaded-portrait*.png`;
the x86 captures are in the paired run's `evidence` directory. Both game
processes exited via harness `quit`. The screenshots are qualitative runtime
evidence only: frame timing and camera state differ, so they do not establish
x86/x64 rendered-pixel equality.

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
the user independently reported no visual issue. FaceGen and unexercised
head/sequence combinations have their separate checks above; all 136 game
head streams are now covered with two sequences, including active neck motion.
`Test-NativeHeadDecode.ps1` automates the three-head raw-coordinate comparison
with the x86 neutral oracle. Its loose explicit-vertex tolerance measures the
known gap rather than accepting it as finished animation; the full
`Test-FaceParity.ps1` remains the strict red/green gate.
