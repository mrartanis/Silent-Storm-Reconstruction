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

## Second pass: 34 non-obvious prefix-grouped files

These decisions use the implementation and its call sites, rather than its
filename. “Later client” means the file can be needed for a playable Linux
client, but does not supply a missing headless world rule. The Windows game
still builds these files; exclusion here applies only to the Linux core.

| File | Observed caller/data edge | Stage-2 disposition |
|---|---|---|
| `2DScene.cpp` | `GRects`/`GfxUtils` 2-D scene construction | later client, graphics |
| `2DSceneSW.cpp` | software rasterization through `GfxBuffers`, `SWTexture`, `Render` | later client, graphics |
| `bmpfile.cpp` | `WriteBMP` is called by `iBase.cpp` for a screenshot | later client, UI/export |
| `BSchemaViewer.cpp` | creates meshes and uses `GView`/`G2DView` | viewer, not game rules |
| `Cache.cpp` | initializes Fibonacci sizes used by the GPU-buffer caches in `GfxBuffers.cpp` | later client, graphics allocator |
| `Camera.cpp` | consumes input and view/UI interfaces; even its AI-map trace is for camera occlusion | later client, camera/input |
| `Cursor.cpp` | cursor hit/appearance via `G2DView` and input bindings | later client, UI/input |
| `FontFormat.cpp` | `CFontFormatInfo` is loaded by `GFont`/`GLocale` and consumed by `GText`/`UIML` | later client, text display resource, not world data |
| `GAutoDetect.cpp` | sets `gfx_*` preset variables from graphics performance | later client, graphics settings |
| `GBinkPlayer.cpp` | Bink frames are uploaded to `NGfx::CTexture`; the game-used FFmpeg/miniaudio codec is already covered in `PORTABILITY-MEDIA.md` | later client video presentation; no second codec port |
| `GMemBuilder.cpp` | `CreateObjectInfo` converts `CMemObject` to render geometry; callers in `G2DView`/`GView` | later client, debug/scene geometry |
| `GMemFormat.cpp` | `CMemGeometry`/`CMemObjectInfo` produce `GfxBuffers`/`GfxRender` geometry | later client, scene geometry |
| `MemObject.cpp` | mesh builders used by `iAIViewer`, `BSchemaViewer`, `GView`, `RWGame`; AI-map `AddHull(CMemObject*)` exists, but no live world caller constructs such a hull (world callers use DB `CAIGeometry`) | later client/viewer; retain DB AI-geometry path in core |
| `RectPacker.cpp` | `PackRects` is called from `GGeometryUtil.cpp` for the lightmap atlas | later client, graphics/lightmap packing |
| `Sound.cpp` | `NSound::CSoundScene` owns music/2-D/3-D playback and FMOD-compatible channels | later Linux client audio; Windows playback already exercised in `PORTABILITY-MEDIA.md` |
| `SoundEffect.cpp` | `CSoundEffect` schedules audio instances on sound channels | later Linux client audio, not world effect rules |
| `SoundFormat.cpp` | `CFileSample2D/3D` load samples for the sound scene | later Linux client audio resource presentation |
| `RWSound.cpp` | `CRenderSound` attaches sounds to the visible-object visitor set | later client audio/visibility |
| `RWGame.cpp` | `CRenderGame` owns the scene and two sound mixers, updates visible entities and weather effects | later client render/sound bridge; `CWorld` remains in core |
| `iBase.cpp` | grabs a framebuffer, camera/view transform and calls `WriteBMP` | later client screen/export infrastructure |
| `iChapterMap.cpp` | interactive chapter map through `GView`/`G2DView`, input and sound | later client; `RPGGlobal` campaign state is separately in core |
| `iGlobalMap.cpp` | interactive global map through `GView`/`G2DView`, input and save manager | later client; map data and campaign state are separately in core |
| `iInterMission.cpp` | interface transition between mission and menu with camera and 2-D view | later client transition, not world simulation |
| `iMain.cpp` | creates the window, graphics scene, screen capture and interface loop | later client platform/boot; headless world boot has its own probe |
| `iMissionExec.cpp` | `CUICmd*Exec` drives camera/locator and sends completion `CCmdInterfaceEvent` to the world | later client script/UI command executor; core event type remains in the world |
| `iPlayerSwitch.cpp` | switches the selected player in `iMission`/`iCommonUI` | later client selection UI |
| `iRenderWorld.cpp` | binds `GView`, `G2DView`, `Sound` and `RWGame` to the current world | later client render/audio bridge |
| `iSaveLoad.cpp` | draws save/load dialogs and screenshots; invokes save manager slots | later client UI; slot logic is separate |
| `iSaveManager.cpp` | Windows profile/slot filesystem implementation; Linux uses `iSaveManagerLinux.cpp` with `NativeSaveManagerTests` | platform-specific implementation, covered by Linux replacement |
| `iScriptScene.cpp` | `CScriptSceneUI`/`CScriptSceneInterface` create a camera, 3-D view and UI template for a scripted scene | later client scene presentation, not Lua/world interpreter |
| `iSpecialView.cpp` | 3-D/2-D special-view interface using `RPGGlobal` data | later client presentation; not campaign-state ownership |
| `iGameStates.cpp` | cursor/input states build `NWorld::CCmd*` for move, attack, use, items, traps and first aid; actual `CanDo`/execution belongs to world command classes | later client command mapping; preserve/test world command semantics separately |
| `PlayerTracker.cpp` | UI tracker creates a human `CSequenceCommander`, adds a world player, then owns camera/selection; `NativeWorldInitProbe.cpp` constructs the same commander/player directly | later client tracker; core AI-perception prerequisite is present in headless boot |
| `UnitTracker.cpp` | UI tracker previews paths, submits `CCmdPath`/`CCmdCancel`, draws path and selection, and keeps skill-change flash baselines; `CanDo` and command execution are in world | later client feedback/command mapping; core path/RPG command semantics separate |

This resolves 34 of the 133 prefix-grouped candidates; 99 still require
per-file confirmation. The three command-building UI trackers above are a
client contract, not evidence that their world commands are missing from the
headless core. Renderer-named sources may still hide CPU game data. The 22
exceptions above remain preliminary until the final cross-check. Do not
declare item 1 closed from this pass.

The stage-2 gate remains the **used non-graphics code and data**, not a
mechanical 338/338 Linux compile, a Linux graphical client, or full Steam
decision parity. The latter belong to later stages.
