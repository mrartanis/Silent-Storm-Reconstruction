# Stage-2 source boundary audit

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
| `aiVolumeCalcer.cpp` | `VolumeCalcerTest` is the only exported caller found | debug/construction, exclude; cross-check below |
| `BSPTree.cpp` | original `aiObjectLoader.cpp` uses `CPrecalcSpheres` instead; `CBSPCollider` replaced by `CPrecalcCollider` | legacy collider, exclude; cross-check below |
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

## Third pass: remaining 59 UI/input files

These are exact file sets (not a wildcard exclusion). The class and include
edges were inspected for each file; the grouping avoids repeating identical
client-layer evidence. Character creation, skill allocation and mission
controls **are game-used features**: this table classifies their UI code,
not their RPG/world rules or original data. Those core responsibilities must
remain covered by the later data/ABI gates.

| Files | Observed responsibility | Stage-2 disposition |
|---|---|---|
| `iAdvFaceGen.cpp`, `iBiographyPanel.cpp`, `iCharacterPanel.cpp`, `iCharGen.cpp`, `iFaceGen.cpp`, `iInventoryPanel.cpp`, `iMedalsPanel.cpp`, `iPerksPanel.cpp`, `iStorePanel.cpp`, `iTeamMngMenu.cpp`, `iUnitPanel.cpp` | game-facing character/team/inventory panels; use `GView`/`G2DView` or `Gfx` and interface controls to display or submit RPG changes | later client UI; game-used FaceGen/RPG data and rules remain core, not dismissed as editor-only |
| `iCommonUI.cpp`, `iCriticalIcons.cpp`, `iDesktopWindow.cpp`, `iLogPanel.cpp`, `iMissionBase.cpp`, `iMissionDlgUI.cpp`, `iMissionMovieUI.cpp`, `iMissionTrailerUI.cpp`, `iMissionUI.cpp`, `iTopPanel.cpp`, `iUnitIconBar.cpp` | tactical HUD, desktop/dialog windows, camera/view, unit status and script/cutscene presentation | later client UI; world commands/events are distinct core contracts |
| `iCluesMenu.cpp`, `iCreditsScreen.cpp`, `iCustomGameMenu.cpp`, `iExitMenu.cpp`, `iHeroMenu.cpp`, `iInGameMenu.cpp`, `iIntroScreen.cpp`, `iLoading.cpp`, `iLoseFake.cpp`, `iMainMenu.cpp`, `iMultiPlayerMenu.cpp`, `iObjectivesMenu.cpp`, `iOptionsMenu.cpp`, `iShowClue.cpp`, `iShowHint.cpp`, `iShowMedal.cpp`, `iShowObjectives.cpp`, `iSideMenu.cpp` | menus, loading/intro, story and objective popups; depend on 2-D view/UI templates or `Gfx` | later client UI; campaign/objective state stays in core |
| `iAIViewer.cpp`, `iRadTest.cpp` | explicit AI visualization/test view through `GView` | debug/viewer, not a game-rule module |
| `iMissionInternal.cpp`, `SWRectLayout.cpp` | empty/precompiled-header-only translation units | exclude as build placeholders |
| `ScreenShot.cpp`, `SplashScreen.cpp`, `SplashScreenDialog.cpp` | framebuffer capture and Win32 splash lifecycle | later client/platform |
| `SWTexture.cpp`, `UIBaseCtrls.cpp`, `UICommCtrls.cpp`, `UIInterface.cpp`, `UIML.cpp`, `UIWindow.cpp`, `UIWrap.cpp`, `WinInputConv.cpp` | software texture, UI controls/layout/markup/window and Win32 input event conversion | later client graphics/input/UI infrastructure |
| `iChapterMapUI.cpp`, `iGlobalMapUI.cpp` | 2-D/3-D map windows built on `GView`, `G2DView` and `Interface` | later client map interfaces; core chapter/global state separate |
| `iMission.cpp` | creates a playable `CMission` with `CWorld`, scene, sound, players, UI and event loop; headless world/party boot is separately exercised by `NativeWorldInitProbe.cpp` | later client mission shell; retain world/party/command contracts in core |
| `iAutoPlay.cpp` | developer `autoplay` console command starts a random mission with publisher-logo UI and exits on input | optional attract/debug mode; not required game core |

Together with the 34 individually traced files, this accounts for all 74
UI/input entries and five sound/render-sound entries from the first-pass
grouping. The three command-building UI trackers above are a client contract,
not evidence that their world commands are missing from the headless core.

## Fourth pass: remaining 40 scene/render/model files

For these files the inspected implementation entry points are scene, GPU,
material, text, terrain image, light, decal or particle operations. This is
**not** a claim that their *referenced data* can be ignored: the game-used
CPU resource paths are separately represented in portable sources and need
the coverage graph in closeout item 2. No light/particle implementation is
resumed by this classification.

| Files | Observed responsibility and core separation | Stage-2 disposition |
|---|---|---|
| `GAnimLight.cpp` | `CLightLoader` parses an animated light and binds it into a scene; an uncommitted CPU-wire test/target already exists and is intentionally preserved | defer further light work to graphics stage; do not include WIP in a stage-2 archive |
| `GBuilding.cpp`, `GGrass.cpp`, `GTerrain.cpp`, `GTerrainTexture.cpp`, `GView.cpp`, `GSceneInternal.cpp`, `GSceneParticles.cpp` | scene buildings, grass sway, terrain mesh/texture, view construction, scene tracing/visibility, particle geometry; `GBuilding`'s game-used building grid/HP dependencies are in headless `BuildingInfo`/`BuildingGrid` (`NATIVE-BUILDING-TERRAIN-LINUX.md`) | later client scene; world collision, destruction and terrain rules stay core |
| `GParticleInfo.cpp`, `GParticles.cpp`, `GPointLightGlow.cpp` | `AddParticles`/particle animation and glow submit render output; game-used effect-record decoding is in portable `GParticleFormat.cpp` (`NATIVE-PARTICLE-RUNTIME-RESOURCES.md`) | later graphics; do not resume particle work in stage 2 |
| `GLightmap.cpp`, `GLightmapCalc.cpp`, `GRenderLight.cpp`, `GShadowMap.cpp`, `GShadowVolume.cpp` | lightmap caches/calculation, rendering lights and shadow geometry; shared persisted lightmap state is isolated in `GLightmapStateWire.h` (`PORTABLE-RENDER-STATE-WIRE.md`) | later graphics; do not resume light work in stage 2 |
| `GDecal.cpp`, `GDecalGeometry.cpp`, `GClipper.cpp`, `GCombiner.cpp`, `GRenderCore.cpp`, `GRenderExecute.cpp`, `GRenderFactor.cpp`, `GTransparent.cpp`, `GRects.cpp`, `GfxUtils.cpp` | decals/shadow meshes, vertex/index combination, render lists/factors, transparent pass and 2-D quads; model/clip CPU data loaders are separated into `GObjectInfo.cpp` and `PortableMeshCodecs.h` (`NATIVE-BUILDING-CLIPPERS.md`) | later graphics, not world hit/collision ownership |
| `GFont.cpp`, `GLocale.cpp`, `GText.cpp` | font resource to screen text/locale formatting | later client UI/text; not game text IDs in `game.db` |
| `GMaterial.cpp`, `GMatShare.cpp`, `GTexture.cpp` | shader/material selection and texture upload/loading for the renderer | later graphics; separately audit referenced texture formats only for future client |
| `Gfx.cpp`, `GfxBuffers.cpp`, `GfxEffects.cpp`, `GfxInternal.cpp`, `GfxRender.cpp`, `GfxShaders.cpp`, `GInit.cpp`, `GPostProcessors.cpp` | D3D9 device/buffers/effects/shaders/init/postprocess (or empty TU) | stage 4 graphics/backend, not portable game core |

## Final cross-check of the 22 exceptions

`aiVolumeCalcer.cpp` has no external call to its volume functions;
`VolumeCalcerTest` returns before its test body, and the only two class
registrations there are commented out. `BSPTree.cpp` does register the old
`CBSPTree` class ID `0x71582160`, but no live game source constructs or calls
it. The original-game `aiObjectLoader.cpp` reads `CPrecalcSpheres` at outer
chunk 9 and per-piece chunk 13, registers `CPrecalcSpheres` as
`0x72813140`, and the collision path uses `CPrecalcCollider`. The old BSP
class can matter to old dev-format saves; cross-save with that format is not
a product requirement. This does **not** exclude AI-geometry and collision
data themselves: they are active core paths.

The other 20 exceptions were cross-checked against callers/registration:
`aiVision` functions are called only by `iAIViewer`; the locker probes in
`aiPositionDebug` have only an absent debug caller; `RPGCover` and
`RPGAttackSession` have no live caller in the current world path (their
retail behavioral convergence is stage 7); `CWaypointsHolder` has no live
constructor or call and `CWorld` owns the active waypoint registry/save
field. `wCheckTooMuchCorpses` is explicitly unwired and would manage visual
visitor density, not corpse/game state. `wInterfaceVisitors` only registers
the visual interface class. `LSHead`/`LSController` own display animation;
`LSHeadPortable` and its tests own the game-used CPU head data. `scriptUI`
contains window Lua APIs; `NATIVE-LUA-USED-SURFACE.md` records the shipped
script-reference check and the retained game-used headless commands. The
remaining exceptions are empty/comment-only, build-only or direct D3D/UI
files as stated in the first table. None supplies an unported, reachable
non-graphics world module.

Closeout item 1 is complete **for the committed Windows `Main_SRC` source
set and the currently established shipped-game paths**. Its result is a
source boundary, not proof of complete data coverage or behavioral parity.
Item 2 must still follow original-game references to effective resources;
if it uncovers a new live decoder edge, reopen the affected row rather than
silently widening the Linux build. The candidate count is not a completion
percentage.

The stage-2 gate remains the **used non-graphics code and data**, not a
mechanical 338/338 Linux compile, a Linux graphical client, or full Steam
decision parity. The latter belong to later stages.
