# Stage-2 source boundary audit (in progress)

The authoritative Windows source set is `set(Main_SRC ...)` in
`sources.cmake`, not every `.cpp` in `Main/`. On 2026-09-28 it contains
338 `.cpp` entries; 155 file names are absent from the committed Linux
`cmake/PortableCore.cmake` source list. This is a review queue, **not** a
count of missing game features. The current working tree additionally has
uncommitted animated-light work, which is excluded from the stage-2 decision.

Reproduce the candidate set by extracting the quoted `.cpp` names from
`Main_SRC` and comparing each `Main/<name>` with the **committed** portable
CMake file (`git show HEAD:cmake/PortableCore.cmake`). The first-pass filename
grouping yields 74 UI/window/input entries, 54
scene/render/model entries, five sound/render-sound entries, and the 22
exceptions below. Filename grouping is only triage; it is not proof of
reachability or exclusion.

| Exception | Current evidence | Stage-2 disposition |
|---|---|---|
| `checkcd.cpp` | disc check, not game data or rules | platform/release, exclude |
| `aiCombatStubs.cpp`, `aiCriterion.cpp` | comment-only historical substrate | exclude unless a live caller appears |
| `aiPositionDebug.cpp` | its banner identifies absent `CPathNetwork::DebugCheck` as sole caller | debug-only, exclude |
| `aiVision.cpp` | `TraceSide`/`GetIndex` calls found in `iAIViewer.cpp` only | editor/debug viewer, exclude |
| `aiVolumeCalcer.cpp` | `VolumeCalcerTest` is the only exported caller found | debug/construction, exclude pending reachability check |
| `BSPTree.cpp` | original `aiObjectLoader.cpp` uses `CPrecalcSpheres` instead; `CBSPCollider` replaced by `CPrecalcCollider` | legacy collider, exclude pending save-type audit |
| `Console.cpp`, `G2DView.cpp` | D3D/UI dependencies | later client stages |
| `LSController.cpp`, `LSHead.cpp` | HUD/FaceGen presentation; game-used CPU head data is in `LSHeadPortable.cpp` and focused tests | leave display/animation integration for later stages |
| `RPGAttackSession.cpp` | only three bookkeeping methods reconstructed; no active caller of `CUnitMissionForMedals` found | not a current stage-2 runtime gap; revisit with medal behavior in stage 7 |
| `RPGCover.cpp` | helper has no active caller; live AI path uses the current cover representation | behavioral convergence in stage 7, not source-count port |
| `wMainWaypoints.cpp` | `CWorld` has active waypoint loading, registry, lookup and save fields in `wMain.cpp/.h`; `CWaypointsHolder` itself has no call site | alternate reconstruction, exclude |
| `scriptUI.cpp` | window-only Lua registration separated from game-used headless bindings (`NATIVE-LUA-USED-SURFACE.md`) | later client stages; retain required script commands in core |
| `scriptVector.cpp`, `wVision.cpp`, `wPlayer.cpp`, `wTurnBased.cpp` | bodies are empty or commented out | exclude |
| `StdAfx.cpp` | precompiled-header translation unit | build-only, exclude |
| `wCheckTooMuchCorpses.cpp`, `wInterfaceVisitors.cpp` | visual visitor-set culling/registration; density helper is not wired to the current world | image/visitor integration later, not stage-2 data loading |

The 133 prefix-grouped files still need per-file confirmation, especially
where a renderer-named source may hide CPU game data. Do not declare item 1
closed from this first pass. The next audit pass will record a concrete
caller or data reference and existing replacement/test for each such file.

The stage-2 gate remains the **used non-graphics code and data**, not a
mechanical 338/338 Linux compile, a Linux graphical client, or full Steam
decision parity. The latter belong to later stages.
