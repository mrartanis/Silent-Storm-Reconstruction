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

Coverage still needed before declaring native facial animation complete:
multiple sequence types (idle, speech, expression masks), overlapping tracks,
all head segments/topologies, saved/reloaded heads, transformable FaceGen GDP
and `.mmt` morphs, texture-weight user items, and a live x64 portrait/head
render compared to x86. The small corpus is an implementation guide, not a
substitute for these cases.

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
36 bones, 419 total vertices, 287 explicitly stored and 132 stored at the
tail as implicit records. Their raw coordinates were compared with the x86
neutral `Process` output. All 132 implicit positions matched at the printed
precision; the maximum differences over the full heads were 0.00906, 0.00938
and 0.00875 for heads 56, 86 and 102. Only the last 22 explicit vertices of
each head exceeded 1e-4; the other source positions matched at that tolerance.
Skipping `ComputePhysics` and `FillUnused` in the x86 probe did not remove
those differences, so the remaining correction is in `Process` or its inputs.
Copying positions is still not a parity implementation.
`NativeHeadDataTests` covers a synthetic valid stream and truncation/invalid
count/index cases under CTest. Neither this decoder nor the test currently
claims to animate a head or satisfy the full x64 parity gate. Bone/muscle
structures, sequence evaluation and FaceGen remain to be implemented natively.
`Test-NativeHeadDecode.ps1` automates the three-head raw-coordinate comparison
with the x86 neutral oracle. Its loose explicit-vertex tolerance measures the
known gap rather than accepting it as finished animation; the full
`Test-FaceParity.ps1` remains the strict red/green gate.
