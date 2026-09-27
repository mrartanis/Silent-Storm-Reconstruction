# Platform-independent data decoding/evaluation used by the native FaceGen path.
# This is deliberately narrower than the Win32 game target; expand this list as
# other engine subsystems lose their Windows dependencies.
add_library(s2_game_objects STATIC
  "${root}/Misc/Basic2.cpp"
  "${root}/Misc/EventsBase.cpp")
target_include_directories(s2_game_objects PUBLIC "${root}/Misc")
target_compile_features(s2_game_objects PUBLIC cxx_std_17)
add_executable(NativeObjectCoreTests "${root}/diagnostics/NativeObjectCoreTests.cpp")
target_link_libraries(NativeObjectCoreTests PRIVATE s2_game_objects)
add_test(NAME NativeObjectCoreTests COMMAND NativeObjectCoreTests)

# The game's own binary memory/file stream layer, shared with Windows FileIO.
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/fileio_include")
file(WRITE "${CMAKE_BINARY_DIR}/fileio_include/stdafx.h" "#include \"${root}/FileIO/StdAfx.h\"\n")
add_library(s2_game_streams STATIC "${root}/FileIO/Streams.cpp")
target_include_directories(s2_game_streams PRIVATE "${CMAKE_BINARY_DIR}/fileio_include")
target_compile_features(s2_game_streams PUBLIC cxx_std_17)
add_executable(NativeStreamsTests "${root}/diagnostics/NativeStreamsTests.cpp")
target_link_libraries(NativeStreamsTests PRIVATE s2_game_streams)
add_test(NAME NativeStreamsTests COMMAND NativeStreamsTests)

# The game's object-graph serializer, including its retail packed-chunk codec.
add_library(s2_game_structure STATIC
  "${root}/FileIO/BasicChunk1.cpp"
  "${root}/FileIO/Cruncher.cpp")
target_include_directories(s2_game_structure PRIVATE "${CMAKE_BINARY_DIR}/fileio_include")
target_link_libraries(s2_game_structure PUBLIC
  s2_game_streams s2_game_objects s2_portable_structure)
target_compile_features(s2_game_structure PUBLIC cxx_std_17)
add_executable(StructureWireProbe "${root}/diagnostics/StructureWireProbe.cpp")
target_link_libraries(StructureWireProbe PRIVATE s2_game_structure)
add_test(NAME StructureWireWrite
  COMMAND StructureWireProbe write "${CMAKE_BINARY_DIR}/StructureWireProbe.bin")
add_test(NAME StructureWireRead
  COMMAND StructureWireProbe read "${CMAKE_BINARY_DIR}/StructureWireProbe.bin")
set_tests_properties(StructureWireWrite PROPERTIES FIXTURES_SETUP structure_wire)
set_tests_properties(StructureWireRead PROPERTIES FIXTURES_REQUIRED structure_wire)
add_executable(NativeCruncherTests "${root}/diagnostics/NativeCruncherTests.cpp")
target_link_libraries(NativeCruncherTests PRIVATE s2_game_structure)
add_test(NAME NativeCruncherTests COMMAND NativeCruncherTests)

# The game's .res package stream, excluding its Windows/editor-only package
# creation and directory rescan path.
add_library(s2_game_resource_package STATIC "${root}/FileIO/FilesPackage.cpp")
target_include_directories(s2_game_resource_package PRIVATE
  "${root}/FileIO" "${root}/Misc")
target_link_libraries(s2_game_resource_package PUBLIC
  s2_game_structure s2_portable_package)
target_compile_features(s2_game_resource_package PUBLIC cxx_std_17)
set(S2_RESOURCE_PACKAGE_PATH "" CACHE FILEPATH "Original .res package for native stream regression")
add_executable(NativeResourcePackageTests
  "${root}/diagnostics/NativeResourcePackageTests.cpp")
target_link_libraries(NativeResourcePackageTests PRIVATE s2_game_resource_package)
if(S2_RESOURCE_PACKAGE_PATH)
  add_test(NAME NativeResourcePackageTests
    COMMAND NativeResourcePackageTests "${S2_RESOURCE_PACKAGE_PATH}")
  get_filename_component(_s2_package_corpus_dir "${S2_RESOURCE_PACKAGE_PATH}" DIRECTORY)
  if(EXISTS "${_s2_package_corpus_dir}/Textures.res" AND
     EXISTS "${_s2_package_corpus_dir}/Sounds.res")
    add_test(NAME NativeResourceCorpusTests
      COMMAND NativeResourcePackageTests --corpus "${_s2_package_corpus_dir}")
    set_tests_properties(NativeResourceCorpusTests PROPERTIES TIMEOUT 900)
  endif()
endif()

# The game's world/camera transform and bound mathematics, without a renderer.
add_library(s2_game_transform STATIC "${root}/Main/Transform.cpp")
target_include_directories(s2_game_transform PRIVATE "${root}/Main" "${root}/Misc" "${root}/FileIO")
target_compile_features(s2_game_transform PUBLIC cxx_std_17)
add_executable(NativeTransformTests "${root}/diagnostics/NativeTransformTests.cpp")
target_link_libraries(NativeTransformTests PRIVATE s2_game_transform)
add_test(NAME NativeTransformTests COMMAND NativeTransformTests)

# The game's terrain-height smoothing kernel; world/path-network ownership follows later.
add_library(s2_game_beta_spline STATIC "${root}/Main/BetaSpline.cpp")
target_include_directories(s2_game_beta_spline PRIVATE "${root}/Main" "${root}/Misc" "${root}/FileIO")
target_compile_features(s2_game_beta_spline PUBLIC cxx_std_17)
add_executable(NativeBetaSplineTests "${root}/diagnostics/NativeBetaSplineTests.cpp")
target_link_libraries(NativeBetaSplineTests PRIVATE s2_game_beta_spline)
add_test(NAME NativeBetaSplineTests COMMAND NativeBetaSplineTests)

# The game's dependency-graph frame/version and deferred object-hold runtime.
add_library(s2_game_dg STATIC "${root}/Main/DG.CPP")
target_include_directories(s2_game_dg PRIVATE "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(s2_game_dg PUBLIC s2_game_structure)
target_compile_features(s2_game_dg PUBLIC cxx_std_17)
add_executable(NativeDGTests "${root}/diagnostics/NativeDGTests.cpp")
target_link_libraries(NativeDGTests PRIVATE s2_game_dg)
add_test(NAME NativeDGTests COMMAND NativeDGTests)

# Original terrain data fields, save tags, and region invalidation atop DG.
add_library(s2_game_terrain_info STATIC "${root}/Main/TerrainInfo.cpp")
target_include_directories(s2_game_terrain_info PRIVATE "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(s2_game_terrain_info PUBLIC s2_game_dg)
target_compile_features(s2_game_terrain_info PUBLIC cxx_std_17)
# The game's original registry and columnar game.db import, excluding the
# SQL/ADO source importer used by editor tools.
add_library(s2_game_database_runtime STATIC "${root}/ADOImport/BasicDB.cpp")
target_include_directories(s2_game_database_runtime PRIVATE
  "${root}/ADOImport" "${root}/FileIO" "${root}/Misc")
target_link_libraries(s2_game_database_runtime PUBLIC
  s2_game_structure s2_portable_database)
target_compile_features(s2_game_database_runtime PUBLIC cxx_std_17)
add_library(s2_game_misc_runtime STATIC
	"${root}/Misc/HPTimer.cpp"
  "${root}/Misc/RandomGen.cpp"
  "${root}/Misc/StrProc.cpp"
  "${root}/Misc/Tools.cpp")
target_include_directories(s2_game_misc_runtime PRIVATE
  "${root}/Misc" "${root}/FileIO")
target_link_libraries(s2_game_misc_runtime PUBLIC s2_game_structure)
target_compile_features(s2_game_misc_runtime PUBLIC cxx_std_17)
add_executable(NativeStrProcTests "${root}/diagnostics/NativeStrProcTests.cpp")
target_link_libraries(NativeStrProcTests PRIVATE s2_game_misc_runtime)
add_test(NAME NativeStrProcTests COMMAND NativeStrProcTests)
# Typed game.db records and the original post-load link builder. Editor-only
# dbinfo/StdAfx translation units are intentionally not part of this target.
add_library(s2_game_dbformat_records STATIC
  "${root}/DBFormat/DataFormat.cpp"
  "${root}/DBFormat/DataAck.cpp"
  "${root}/DBFormat/DataAI.cpp"
  "${root}/DBFormat/DataCamera.cpp"
  "${root}/DBFormat/DataChest.cpp"
  "${root}/DBFormat/DataConst.cpp"
  "${root}/DBFormat/DataDifficulty.cpp"
  "${root}/DBFormat/DataFaceGen.cpp"
  "${root}/DBFormat/DataInterface.cpp"
  "${root}/DBFormat/DataMap.cpp"
  "${root}/DBFormat/DataMisc.cpp"
  "${root}/DBFormat/DataPerk.cpp"
  "${root}/DBFormat/DataPhys.cpp"
  "${root}/DBFormat/DataRPG.cpp"
  "${root}/DBFormat/DataRPGTmp.cpp"
  "${root}/DBFormat/DataRpgConstants.cpp"
  "${root}/DBFormat/DataScenario.cpp"
  "${root}/DBFormat/DataScript.cpp")
target_include_directories(s2_game_dbformat_records PRIVATE
  "${root}/DBFormat" "${root}/ADOImport" "${root}/FileIO" "${root}/Misc" "${root}/Main")
target_link_libraries(s2_game_dbformat_records PUBLIC
  s2_game_database_runtime s2_game_misc_runtime)
target_compile_features(s2_game_dbformat_records PUBLIC cxx_std_17)
set(S2_GAME_DB_PATH "" CACHE FILEPATH "Path to an original game.db for native load regression")
add_executable(NativeGameDatabaseLoadTests
  "${root}/diagnostics/NativeGameDatabaseLoadTests.cpp")
target_link_libraries(NativeGameDatabaseLoadTests PRIVATE
  "-Wl,--whole-archive" s2_game_dbformat_records "-Wl,--no-whole-archive")
if(S2_GAME_DB_PATH)
  add_test(NAME NativeGameDatabaseLoadTests
    COMMAND NativeGameDatabaseLoadTests "${S2_GAME_DB_PATH}")
endif()
add_executable(NativeMapDatabaseTests
  "${root}/diagnostics/NativeMapDatabaseTests.cpp")
target_link_libraries(NativeMapDatabaseTests PRIVATE
  "-Wl,--whole-archive" s2_game_dbformat_records "-Wl,--no-whole-archive")
if(S2_GAME_DB_PATH)
  add_test(NAME NativeMapDatabaseTests
    COMMAND NativeMapDatabaseTests "${S2_GAME_DB_PATH}")
endif()
# The typed database now supplies the material/armor vtables required by
# STerrainInfo's saved CDBPtr fields. Link the original region/version test.
add_executable(NativeTerrainInfoTests "${root}/diagnostics/NativeTerrainInfoTests.cpp")
target_link_libraries(NativeTerrainInfoTests PRIVATE s2_game_terrain_info
  "-Wl,--whole-archive" s2_game_dbformat_records "-Wl,--no-whole-archive")
add_test(NAME NativeTerrainInfoTests COMMAND NativeTerrainInfoTests)
if(S2_GAME_DB_PATH)
  add_test(NAME NativeTerrainInfoDatabaseTests
    COMMAND NativeTerrainInfoTests "${S2_GAME_DB_PATH}")
endif()

# The original world height-cache translation unit, including its floor
# rasterization code. Its path-network ownership is the next link boundary.
add_library(s2_game_height_layers STATIC "${root}/Main/wHeightLayers.cpp")
target_include_directories(s2_game_height_layers PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat" "${root}/ADOImport")
target_link_libraries(s2_game_height_layers PUBLIC
  s2_game_terrain_info s2_game_beta_spline)
target_compile_features(s2_game_height_layers PUBLIC cxx_std_17)
target_compile_options(s2_game_height_layers PRIVATE
  -ffunction-sections -fdata-sections -fno-sanitize=vptr)
add_executable(NativeHeightLayersTests "${root}/diagnostics/NativeHeightLayersTests.cpp")
target_link_libraries(NativeHeightLayersTests PRIVATE s2_game_height_layers)
target_link_options(NativeHeightLayersTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeHeightLayersTests COMMAND NativeHeightLayersTests)
add_executable(NativePoolTests "${root}/diagnostics/NativePoolTests.cpp")
target_compile_features(NativePoolTests PUBLIC cxx_std_17)
add_test(NAME NativePoolTests COMMAND NativePoolTests)

# Original path-network implementation required by height-layer tile input.
add_library(s2_game_ai_grid STATIC "${root}/Main/aiGrid.cpp")
target_include_directories(s2_game_ai_grid PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat" "${root}/ADOImport")
target_compile_features(s2_game_ai_grid PUBLIC cxx_std_17)
target_compile_options(s2_game_ai_grid PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_ai_position STATIC "${root}/Main/aiPosition.cpp")
target_include_directories(s2_game_ai_position PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(s2_game_ai_position PUBLIC s2_game_ai_grid s2_game_structure)
target_compile_features(s2_game_ai_position PUBLIC cxx_std_17)
add_library(s2_game_ai_locker STATIC "${root}/Main/aiLocker.cpp")
target_include_directories(s2_game_ai_locker PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat" "${root}/ADOImport")
target_link_libraries(s2_game_ai_locker PUBLIC s2_game_ai_grid)
target_compile_features(s2_game_ai_locker PUBLIC cxx_std_17)
add_library(s2_game_ai_pass_jobs STATIC "${root}/Main/aiPassCalcJob.cpp")
target_include_directories(s2_game_ai_pass_jobs PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat" "${root}/ADOImport")
target_link_libraries(s2_game_ai_pass_jobs PUBLIC s2_game_ai_grid)
target_compile_features(s2_game_ai_pass_jobs PUBLIC cxx_std_17)
add_library(s2_game_ai_calculators STATIC
  "${root}/Main/aiPassCalcer.cpp"
  "${root}/Main/aiMovesCalcer.cpp")
target_include_directories(s2_game_ai_calculators PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat" "${root}/ADOImport")
target_link_libraries(s2_game_ai_calculators PUBLIC s2_game_ai_grid)
target_compile_features(s2_game_ai_calculators PUBLIC cxx_std_17)
add_library(s2_game_ai_render STATIC "${root}/Main/aiRender.cpp")
target_include_directories(s2_game_ai_render PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat" "${root}/ADOImport")
target_link_libraries(s2_game_ai_render PUBLIC s2_game_ai_grid)
target_compile_features(s2_game_ai_render PUBLIC cxx_std_17)
add_library(s2_game_ai_colourer STATIC "${root}/Main/aiColourer.cpp")
target_include_directories(s2_game_ai_colourer PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat" "${root}/ADOImport")
target_link_libraries(s2_game_ai_colourer PUBLIC s2_game_ai_grid)
target_compile_features(s2_game_ai_colourer PUBLIC cxx_std_17)
add_library(s2_game_ai_collision STATIC
  "${root}/Main/aiCollider.cpp"
  "${root}/Main/aiObject.cpp"
  "${root}/Main/SuperCollider.cpp"
  "${root}/Main/VolumeContainer.cpp"
  "${root}/Main/phCollider.cpp")
target_include_directories(s2_game_ai_collision PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat" "${root}/ADOImport")
target_link_libraries(s2_game_ai_collision PUBLIC s2_game_ai_grid s2_game_transform)
target_compile_features(s2_game_ai_collision PUBLIC cxx_std_17)
add_library(s2_game_ai_geometry_loader STATIC
  "${root}/Main/aiObjectLoader.cpp")
target_include_directories(s2_game_ai_geometry_loader PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat")
target_link_libraries(s2_game_ai_geometry_loader PUBLIC
  s2_game_ai_collision s2_game_resource_loader s2_game_basic_share)
target_compile_features(s2_game_ai_geometry_loader PUBLIC cxx_std_17)
target_compile_options(s2_game_ai_geometry_loader PRIVATE
  -ffunction-sections -fdata-sections)
add_library(s2_game_ai_log STATIC "${root}/MiscDll/LogStream.cpp")
target_include_directories(s2_game_ai_log PRIVATE
  "${root}/MiscDll" "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(s2_game_ai_log PUBLIC s2_game_misc_runtime)
target_compile_features(s2_game_ai_log PUBLIC cxx_std_17)
add_library(s2_game_console STATIC
  "${root}/MiscDll/Commands.cpp"
  "${root}/FileIO/LinuxUserData.cpp")
target_include_directories(s2_game_console PRIVATE
  "${root}/MiscDll" "${root}/FileIO" "${root}/Misc" "${root}/Main")
target_link_libraries(s2_game_console PUBLIC
  s2_game_ai_log s2_game_misc_runtime s2_game_structure
  s2_portable_user_paths)
target_compile_features(s2_game_console PUBLIC cxx_std_17)
add_library(s2_game_rpg_execution STATIC
  "${root}/Main/RPGUnit.cpp"
  "${root}/Main/RPGMerc.cpp"
  "${root}/Main/RPGItemSet.cpp"
  "${root}/Main/RPGAttackMech.cpp"
  "${root}/Main/rpgPerk.cpp"
  "${root}/Main/rpgGlobal.cpp")
target_include_directories(s2_game_rpg_execution PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_rpg_execution PUBLIC cxx_std_17)
target_compile_options(s2_game_rpg_execution PRIVATE
  -ffunction-sections -fdata-sections)
add_executable(NativeAttackRulesTests
  "${root}/diagnostics/NativeAttackRulesTests.cpp")
target_link_libraries(NativeAttackRulesTests PRIVATE
  s2_game_rpg_execution s2_game_dbformat_records
  s2_game_structure s2_game_objects)
target_link_options(NativeAttackRulesTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeAttackRulesTests COMMAND NativeAttackRulesTests)
add_executable(NativePerkPointsTests
  "${root}/diagnostics/NativePerkPointsTests.cpp")
target_link_libraries(NativePerkPointsTests PRIVATE
  s2_game_rpg_execution s2_game_dbformat_records
  s2_game_ai_log s2_game_structure s2_game_objects)
target_link_options(NativePerkPointsTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativePerkPointsTests COMMAND NativePerkPointsTests)
add_executable(NativeConsoleConfigTests
  "${root}/diagnostics/NativeConsoleConfigTests.cpp")
target_link_libraries(NativeConsoleConfigTests PRIVATE s2_game_console)
add_test(NAME NativeConsoleConfigTests COMMAND NativeConsoleConfigTests)
set_tests_properties(NativeConsoleConfigTests PROPERTIES
  ENVIRONMENT "S2_USER_DATA_DIR=${CMAKE_BINARY_DIR}/native-console-user")
add_library(s2_game_save_manager STATIC "${root}/Main/iSaveManagerLinux.cpp")
target_include_directories(s2_game_save_manager PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/MiscDll")
target_link_libraries(s2_game_save_manager PUBLIC
  s2_game_console s2_game_streams s2_portable_user_paths)
target_compile_features(s2_game_save_manager PUBLIC cxx_std_17)
add_executable(NativeSaveManagerTests
  "${root}/diagnostics/NativeSaveManagerTests.cpp")
target_link_libraries(NativeSaveManagerTests PRIVATE s2_game_save_manager)
add_test(NAME NativeSaveManagerTests COMMAND NativeSaveManagerTests)
set_tests_properties(NativeSaveManagerTests PROPERTIES
  ENVIRONMENT "S2_USER_DATA_DIR=${CMAKE_BINARY_DIR}/native-save-user")
add_library(s2_game_world_object STATIC "${root}/Main/wOSBase.cpp")
target_include_directories(s2_game_world_object PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll")
target_link_libraries(s2_game_world_object PUBLIC
  s2_game_ai_collision s2_game_ai_log s2_game_transform)
target_compile_features(s2_game_world_object PUBLIC cxx_std_17)
# vptr metadata retains every world/DB RTTI reference even when the link
# probe discards unrelated world-object methods. Keep ASan and other UBSan
# checks; remove this exception when the complete world graph is linked.
target_compile_options(s2_game_world_object PRIVATE
  -ffunction-sections -fdata-sections -fno-sanitize=vptr)
# The original world coordinator now compiles as one Linux translation unit.
# This is a compile boundary, not yet an executable world: its render, UI,
# mission and script bindings still need to join the game link graph.
add_library(s2_game_world STATIC
  "${root}/Main/wMain.cpp"
  "${root}/Main/wMainMoves.cpp"
  "${root}/Main/wMainTrace.cpp"
  "${root}/Main/wMainPath.cpp"
  "${root}/Main/wUICommands.cpp")
target_include_directories(s2_game_world PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_world PUBLIC cxx_std_17)
target_compile_options(s2_game_world PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_unit_execution STATIC
  "${root}/Main/wUnitAttack.cpp"
  "${root}/Main/wUnitExec.cpp"
  "${root}/Main/wUnitMove.cpp"
  "${root}/Main/wUnitQueue.cpp"
  "${root}/Main/wUnitAttackExec.cpp")
target_include_directories(s2_game_unit_execution PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_unit_execution PUBLIC cxx_std_17)
target_compile_options(s2_game_unit_execution PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_human_reach STATIC "${root}/Main/wHumanReach.cpp")
target_include_directories(s2_game_human_reach PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_compile_features(s2_game_human_reach PUBLIC cxx_std_17)
target_compile_options(s2_game_human_reach PRIVATE -ffunction-sections -fdata-sections)
add_executable(NativeHumanReachTests
  "${root}/diagnostics/NativeHumanReachTests.cpp")
target_link_libraries(NativeHumanReachTests PRIVATE
  s2_game_human_reach)
target_link_options(NativeHumanReachTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeHumanReachTests COMMAND NativeHumanReachTests)
add_library(s2_game_world_entities STATIC
  "${root}/Main/wHintsFunc.cpp"
  "${root}/Main/wMisc.cpp"
  "${root}/Main/wDebris.cpp"
  "${root}/Main/wMine.cpp"
  "${root}/Main/wObject.cpp")
target_include_directories(s2_game_world_entities PRIVATE
  "${CMAKE_BINARY_DIR}/main_case_include"
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_world_entities PUBLIC cxx_std_17)
target_compile_options(s2_game_world_entities PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_world_events STATIC
  "${root}/Main/wAckBase.cpp"
  "${root}/Main/wAck.cpp"
  "${root}/Main/wPocket.cpp"
  "${root}/Main/wUnitGroup.cpp"
  "${root}/Main/wBullet.cpp"
  "${root}/Main/wGrenade.cpp"
  "${root}/Main/wKnife.cpp"
  "${root}/Main/wRocket.cpp"
  "${root}/Main/wExplTracker.cpp")
target_include_directories(s2_game_world_events PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_world_events PUBLIC cxx_std_17)
target_compile_options(s2_game_world_events PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_world_gameplay STATIC
  "${root}/Main/wBuilding.cpp"
  "${root}/Main/wDecal.cpp"
  "${root}/Main/wTerrain.cpp"
  "${root}/Main/wInterface.cpp"
  "${root}/Main/InventoryUnit.cpp"
  "${root}/Main/wDialog.cpp"
  "${root}/Main/wInformCorpseStop.cpp"
  "${root}/Main/wExplosionPerks.cpp")
target_include_directories(s2_game_world_gameplay PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_world_gameplay PUBLIC cxx_std_17)
target_compile_options(s2_game_world_gameplay PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_scene_data STATIC
  "${root}/Main/GGeometryCore.cpp"
  "${root}/Main/aiTerrain.cpp"
  "${root}/Main/GBind.cpp"
  "${root}/Main/GMesh.cpp")
target_include_directories(s2_game_scene_data PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_scene_data PUBLIC cxx_std_17)
target_compile_options(s2_game_scene_data PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_combiner_core STATIC "${root}/Main/GCombinerCore.cpp")
target_include_directories(s2_game_combiner_core PRIVATE
  "${CMAKE_BINARY_DIR}/main_case_include"
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll")
target_link_libraries(s2_game_combiner_core PUBLIC
  s2_game_dg s2_game_structure s2_game_objects)
target_compile_features(s2_game_combiner_core PUBLIC cxx_std_17)
target_compile_options(s2_game_combiner_core PRIVATE -ffunction-sections -fdata-sections)
add_executable(NativeCombinerCoreTests
  "${root}/diagnostics/NativeCombinerCoreTests.cpp")
target_include_directories(NativeCombinerCoreTests PRIVATE
  "${CMAKE_BINARY_DIR}/main_case_include" "${root}/Main")
target_link_libraries(NativeCombinerCoreTests PRIVATE s2_game_combiner_core)
target_link_options(NativeCombinerCoreTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeCombinerCoreTests COMMAND NativeCombinerCoreTests)
add_library(s2_game_scene_part_core STATIC "${root}/Main/GScenePartCore.cpp")
target_include_directories(s2_game_scene_part_core PRIVATE
  "${CMAKE_BINARY_DIR}/main_case_include"
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll")
target_link_libraries(s2_game_scene_part_core PUBLIC s2_game_combiner_core)
target_compile_features(s2_game_scene_part_core PUBLIC cxx_std_17)
target_compile_options(s2_game_scene_part_core PRIVATE -ffunction-sections -fdata-sections)
add_executable(NativeScenePartCoreTests
  "${root}/diagnostics/NativeScenePartCoreTests.cpp")
target_include_directories(NativeScenePartCoreTests PRIVATE
  "${CMAKE_BINARY_DIR}/main_case_include" "${root}/Main")
target_link_libraries(NativeScenePartCoreTests PRIVATE
  -Wl,--whole-archive s2_game_scene_part_core -Wl,--no-whole-archive)
target_link_options(NativeScenePartCoreTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeScenePartCoreTests COMMAND NativeScenePartCoreTests)
add_library(s2_game_scene_serialization STATIC
  "${root}/Main/GSceneUtils.cpp"
  "${root}/Main/GDecalTarget.cpp"
  "${root}/Main/DebugParticles.cpp")
target_include_directories(s2_game_scene_serialization PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_link_libraries(s2_game_scene_serialization PUBLIC s2_game_scene_part_core)
target_compile_features(s2_game_scene_serialization PUBLIC cxx_std_17)
target_compile_options(s2_game_scene_serialization PRIVATE -ffunction-sections -fdata-sections)
add_executable(NativeRenderStateWireTests
  "${root}/diagnostics/NativeRenderStateWireTests.cpp")
target_include_directories(NativeRenderStateWireTests PRIVATE
  "${CMAKE_BINARY_DIR}/main_case_include"
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script")
target_compile_features(NativeRenderStateWireTests PRIVATE cxx_std_17)
add_test(NAME NativeRenderStateWireTests COMMAND NativeRenderStateWireTests)
add_executable(NativeSceneClassIDsTests
  "${root}/diagnostics/NativeSceneClassIDsTests.cpp"
  "${root}/Main/GSceneUtils.cpp")
target_include_directories(NativeSceneClassIDsTests PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat")
target_link_libraries(NativeSceneClassIDsTests PRIVATE
  -Wl,--start-group s2_game_transform
  s2_game_dg s2_game_structure s2_game_streams s2_game_objects
  s2_portable_structure -Wl,--end-group)
target_link_options(NativeSceneClassIDsTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeSceneClassIDsTests COMMAND NativeSceneClassIDsTests)
add_executable(NativeGeometryCoreTests
  "${root}/diagnostics/NativeGeometryCoreTests.cpp")
target_include_directories(NativeGeometryCoreTests PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(NativeGeometryCoreTests PRIVATE s2_game_scene_data)
target_link_options(NativeGeometryCoreTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeGeometryCoreTests COMMAND NativeGeometryCoreTests)
add_library(s2_game_scenario_scripts STATIC
  "${root}/Main/scScenarioTracker.cpp"
  "${root}/Main/scFlowChartItems.cpp"
  "${root}/Main/scFlowChart.cpp"
  "${root}/Main/scCommands.cpp"
  "${root}/Main/scriptCommon.cpp"
  "${root}/Main/A5Script.cpp"
  "${root}/Main/scriptPtr.cpp")
target_include_directories(s2_game_scenario_scripts PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_link_libraries(s2_game_scenario_scripts PUBLIC s2_portable_package)
target_compile_features(s2_game_scenario_scripts PUBLIC cxx_std_17)
target_compile_options(s2_game_scenario_scripts PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_script_bindings STATIC
  "${root}/Main/ScriptFunctions.cpp"
  "${root}/Main/scriptDialog.cpp"
  "${root}/Main/scriptDiplomacy.cpp"
  "${root}/Main/scriptObject.cpp"
  "${root}/Main/scriptPosition.cpp"
  "${root}/Main/scriptRoute.cpp"
  "${root}/Main/scriptScenario.cpp"
  "${root}/Main/scriptSequence.cpp"
  "${root}/Main/scriptTemplate.cpp"
  "${root}/Main/scriptUnit.cpp"
  "${root}/Main/scriptUnitGroup.cpp")
target_include_directories(s2_game_script_bindings PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_script_bindings PUBLIC cxx_std_17)
target_compile_options(s2_game_script_bindings PRIVATE -ffunction-sections -fdata-sections)
# Original AI commander, route, reaction, and nearest-position units.
# These compile as the next game-used dependency group; complete Linux
# AI/world execution still requires the remaining map and mission modules.
add_library(s2_game_ai_routes STATIC
  "${root}/Main/aiCommander.cpp"
  "${root}/Main/aiRoute.cpp"
  "${root}/Main/aiRouteLogic.cpp"
  "${root}/Main/aiRouteMisc.cpp"
  "${root}/Main/aiReaction.cpp"
  "${root}/Main/aiReactions.cpp"
  "${root}/Main/aiNearestPosition.cpp"
  "${root}/Main/aiTaskCommand.cpp"
  "${root}/Main/aistate.cpp"
  "${root}/Main/aiUnit.cpp"
  "${root}/Main/aiPlayer.cpp"
  "${root}/Main/aiEvent.cpp")
target_include_directories(s2_game_ai_routes PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_ai_routes PUBLIC cxx_std_17)
target_compile_options(s2_game_ai_routes PRIVATE -ffunction-sections -fdata-sections)
# Game-used AI perception, inventory, combat decisions, and voxel tracing.
# A static archive is not a live Linux AI turn until the world graph links.
add_library(s2_game_ai_perception STATIC
  "${root}/Main/aiThreatTracker.cpp"
  "${root}/Main/aiUnitState.cpp"
  "${root}/Main/aiMisc.cpp"
  "${root}/Main/aiInventory.cpp"
  "${root}/Main/aiCombatLogic.cpp"
  "${root}/Main/aiDefenceReaction.cpp"
  "${root}/Main/aiAssassinReaction.cpp"
  "${root}/Main/aiFearReaction.cpp"
  "${root}/Main/aiGuardReaction.cpp"
  "${root}/Main/aiActionBase.cpp"
  "${root}/Main/aiMoveAction.cpp"
  "${root}/Main/aiTrace.cpp"
  "${root}/Main/aiVoxelRender.cpp"
  "${root}/Main/aiStability.cpp")
target_include_directories(s2_game_ai_perception PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_ai_perception PUBLIC cxx_std_17)
target_compile_options(s2_game_ai_perception PRIVATE -ffunction-sections -fdata-sections)
# Original combat action, weapon, place-source, log and path-job units.
add_library(s2_game_ai_actions STATIC
  "${root}/Main/aiScriptLogic.cpp"
  "${root}/Main/aiScriptReaction.cpp"
  "${root}/Main/aiActions.cpp"
  "${root}/Main/aiWeapon.cpp"
  "${root}/Main/aiActionPlaceSource.cpp"
  "${root}/Main/AILog.cpp"
  "${root}/Main/aiInterval.cpp"
  "${root}/Main/aiJob.cpp"
  "${root}/Main/aiMultiMoves.cpp"
  "${root}/Main/aiCombatLog.cpp"
  "${root}/Main/aiMoves.cpp"
  "${root}/Main/aiPath.cpp"
  "${root}/Main/aiSmoothPath.cpp"
  "${root}/Main/aiChoosePlace.cpp"
  "${root}/Main/aiSnipeAction.cpp"
  "${root}/Main/aiLootAction.cpp"
  "${root}/Main/aiHeavyGunAction.cpp")
target_include_directories(s2_game_ai_actions PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_ai_actions PUBLIC cxx_std_17)
target_compile_options(s2_game_ai_actions PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_rpg_combat STATIC
	"${root}/Main/RPGBuilding.cpp"
  "${root}/Main/RPGToHit.cpp"
  "${root}/Main/RPGCritical.cpp"
  "${root}/Main/RPGBullet.cpp"
  "${root}/Main/RPGStatInfo.cpp")
target_include_directories(s2_game_rpg_combat PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_rpg_combat PUBLIC cxx_std_17)
target_compile_options(s2_game_rpg_combat PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_rpg_inventory_vision STATIC
  "${root}/Main/RPGItemMap.cpp"
  "${root}/Main/RPGMedals.cpp"
  "${root}/Main/RPGInventory.cpp"
  "${root}/Main/RPGVision.cpp")
target_include_directories(s2_game_rpg_inventory_vision PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_rpg_inventory_vision PUBLIC cxx_std_17)
target_compile_options(s2_game_rpg_inventory_vision PRIVATE -ffunction-sections -fdata-sections)
add_executable(NativeRPGItemMapTests
  "${root}/diagnostics/NativeRPGItemMapTests.cpp")
target_link_libraries(NativeRPGItemMapTests PRIVATE
  s2_game_rpg_inventory_vision s2_game_structure s2_game_objects)
target_link_options(NativeRPGItemMapTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeRPGItemMapTests COMMAND NativeRPGItemMapTests)
add_executable(NativeAIIntervalTests
  "${root}/diagnostics/NativeAIIntervalTests.cpp")
target_link_libraries(NativeAIIntervalTests PRIVATE
  s2_game_ai_actions s2_game_objects)
target_link_options(NativeAIIntervalTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeAIIntervalTests COMMAND NativeAIIntervalTests)
add_executable(NativeAITraceSphereTests
  "${root}/diagnostics/NativeAITraceSphereTests.cpp")
target_link_libraries(NativeAITraceSphereTests PRIVATE
  s2_game_ai_perception s2_game_transform s2_game_structure s2_game_objects)
target_link_options(NativeAITraceSphereTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeAITraceSphereTests COMMAND NativeAITraceSphereTests)
add_executable(NativeExplosionVoxelRendererTests
  "${root}/diagnostics/NativeExplosionVoxelRendererTests.cpp")
target_link_libraries(NativeExplosionVoxelRendererTests PRIVATE
  s2_game_ai_perception s2_game_transform s2_game_structure s2_game_objects)
target_link_options(NativeExplosionVoxelRendererTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeExplosionVoxelRendererTests COMMAND NativeExplosionVoxelRendererTests)
# Original AI map and RPG mission state are game-used prerequisites for
# executing a Linux world. Static archives alone do not close the link.
add_library(s2_game_ai_map STATIC "${root}/Main/aiMap.cpp")
target_include_directories(s2_game_ai_map PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_ai_map PUBLIC cxx_std_17)
target_compile_options(s2_game_ai_map PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_rpg_mission STATIC
  "${root}/Main/RPGGame.cpp"
  "${root}/Main/RPGStore.cpp"
  "${root}/Main/RPGDiplomacy.cpp"
  "${root}/Main/RPGObject.cpp"
  "${root}/Main/RPGUnitMission.cpp")
target_include_directories(s2_game_rpg_mission PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll" "${root}/Script"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_game_rpg_mission PUBLIC cxx_std_17)
target_compile_options(s2_game_rpg_mission PRIVATE -ffunction-sections -fdata-sections)
add_executable(NativeDiplomacyBitsTests
  "${root}/diagnostics/NativeDiplomacyBitsTests.cpp")
target_link_libraries(NativeDiplomacyBitsTests PRIVATE
  s2_game_rpg_mission s2_game_dbformat_records
  s2_game_structure s2_game_objects)
target_link_options(NativeDiplomacyBitsTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeDiplomacyBitsTests COMMAND NativeDiplomacyBitsTests)
# The original command-driven AI logic base. It compiles on Linux, but
# executable linking still needs the world unit server and command queue;
# do not confuse this compile boundary with live AI execution.
add_library(s2_game_ai_logic STATIC "${root}/Main/aiLogic.cpp")
target_include_directories(s2_game_ai_logic PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll")
target_compile_features(s2_game_ai_logic PUBLIC cxx_std_17)
target_compile_options(s2_game_ai_logic PRIVATE -ffunction-sections -fdata-sections)
# The original server-side unit command/executor translation unit. Its
# dependencies on the full world are not linked into Linux game execution yet.
add_library(s2_game_unit_server STATIC "${root}/Main/wUnitServer.cpp")
target_include_directories(s2_game_unit_server PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll")
target_compile_features(s2_game_unit_server PUBLIC cxx_std_17)
target_compile_options(s2_game_unit_server PRIVATE -ffunction-sections -fdata-sections)
# The adjacent original unit base and command-state implementations. These
# compile on Linux, but still require the live world graph to link and run.
add_library(s2_game_dumb_unit STATIC "${root}/Main/wDumbUnit.cpp")
target_include_directories(s2_game_dumb_unit PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll")
target_compile_features(s2_game_dumb_unit PUBLIC cxx_std_17)
target_compile_options(s2_game_dumb_unit PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_unit_states STATIC "${root}/Main/wUnitStates.cpp")
target_include_directories(s2_game_unit_states PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll")
target_compile_features(s2_game_unit_states PUBLIC cxx_std_17)
target_compile_options(s2_game_unit_states PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_unit_animator STATIC "${root}/Main/wAnimation.cpp")
target_include_directories(s2_game_unit_animator PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll")
target_compile_features(s2_game_unit_animator PUBLIC cxx_std_17)
target_compile_options(s2_game_unit_animator PRIVATE -ffunction-sections -fdata-sections)
# Original skeleton, terrain, path, and particle animation used by units.
add_library(s2_game_animation_runtime STATIC
  "${root}/Main/GAnimBase.cpp"
  "${root}/Main/GAnimFormat.cpp"
  "${root}/Main/GAnimation.cpp"
  "${root}/Main/GSkeleton.cpp"
  "${root}/Main/GAnimTerrain.cpp"
  "${root}/Main/GAnimPath.cpp"
  "${root}/Main/GAnimParticles.cpp")
target_include_directories(s2_game_animation_runtime PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll")
target_compile_features(s2_game_animation_runtime PUBLIC cxx_std_17)
target_compile_options(s2_game_animation_runtime PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_ai_height STATIC "${root}/Main/aiHeight.cpp")
target_include_directories(s2_game_ai_height PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll")
target_compile_features(s2_game_ai_height PUBLIC cxx_std_17)
target_compile_options(s2_game_ai_height PRIVATE -ffunction-sections -fdata-sections)
add_executable(NativeAnimationPathTests
  "${root}/diagnostics/NativeAnimationPathTests.cpp")
target_link_libraries(NativeAnimationPathTests PRIVATE
  s2_game_animation_runtime s2_game_dg s2_game_structure s2_game_objects)
target_link_options(NativeAnimationPathTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeAnimationPathTests COMMAND NativeAnimationPathTests)
# Actual world-to-unit command wrapper used by the AI command queue.
add_library(s2_game_command_bridge STATIC "${root}/Main/wCommandBridge.cpp")
target_include_directories(s2_game_command_bridge PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport")
target_compile_features(s2_game_command_bridge PUBLIC cxx_std_17)
add_library(s2_game_unit_commands STATIC "${root}/Main/wUnitCommands.cpp")
target_include_directories(s2_game_unit_commands PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport")
target_compile_features(s2_game_unit_commands PUBLIC cxx_std_17)
add_executable(NativeCommandBridgeTests
  "${root}/diagnostics/NativeCommandBridgeTests.cpp")
target_link_libraries(NativeCommandBridgeTests PRIVATE
  s2_game_command_bridge s2_game_unit_commands
  s2_game_structure s2_game_objects)
add_test(NAME NativeCommandBridgeTests COMMAND NativeCommandBridgeTests)
add_executable(NativeAILogTests "${root}/diagnostics/NativeAILogTests.cpp")
target_link_libraries(NativeAILogTests PRIVATE s2_game_ai_log)
add_test(NAME NativeAILogTests COMMAND NativeAILogTests)
add_executable(NativeHeightNetworkTests
  "${root}/diagnostics/NativeHeightNetworkTests.cpp")
target_link_libraries(NativeHeightNetworkTests PRIVATE
  -Wl,--start-group
  s2_game_height_layers s2_game_ai_position s2_game_ai_locker
  s2_game_ai_pass_jobs s2_game_ai_calculators s2_game_ai_render
  s2_game_ai_colourer s2_game_ai_collision s2_game_ai_log
  s2_game_world_object s2_game_ai_grid s2_game_terrain_info
  s2_game_dg s2_game_structure s2_game_streams s2_game_objects
  s2_portable_structure s2_game_beta_spline s2_game_transform
  s2_game_misc_runtime
  -Wl,--end-group)
target_link_options(NativeHeightNetworkTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeHeightNetworkTests COMMAND NativeHeightNetworkTests)

# Original mission-template traversal and map assembly, adjacent to the
# already linked typed map records and path network.
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/main_case_include")
file(WRITE "${CMAKE_BINARY_DIR}/main_case_include/DG.h"
  "#include \"${root}/Main/DG.H\"\n")
file(WRITE "${CMAKE_BINARY_DIR}/main_case_include/dg.h"
  "#include \"${root}/Main/DG.H\"\n")
add_library(s2_game_map_build STATIC "${root}/Main/MapBuild.cpp")
target_include_directories(s2_game_map_build PRIVATE
  "${CMAKE_BINARY_DIR}/main_case_include"
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll")
target_link_libraries(s2_game_map_build PUBLIC
  s2_game_dbformat_records s2_game_ai_grid s2_game_terrain_info
  s2_game_building_grid s2_game_building_info s2_game_building_internal
  s2_game_make_building s2_game_map_terrain)
target_compile_features(s2_game_map_build PUBLIC cxx_std_17)
target_compile_options(s2_game_map_build PRIVATE -ffunction-sections -fdata-sections)
option(S2_ENABLE_MISSION_MAP_PROBE "Link the original game mission builder probe" ON)
if(S2_ENABLE_MISSION_MAP_PROBE)
  add_executable(NativeMissionMapProbe
    "${root}/diagnostics/NativeMissionMapProbe.cpp")
  target_link_libraries(NativeMissionMapProbe PRIVATE
    -Wl,--start-group s2_game_map_build s2_game_lua s2_game_make_building
    s2_game_building_internal s2_game_building_info s2_game_building_grid
    s2_game_building_clip s2_game_map_terrain s2_game_ai_geometry_loader
    s2_game_ai_collision s2_game_ai_grid s2_game_ai_position
    s2_game_ai_locker s2_game_ai_pass_jobs s2_game_ai_calculators
    s2_game_ai_colourer s2_game_ai_render s2_game_ai_log
    s2_game_world_object s2_game_ai_waypoint
    s2_game_terrain_info
    s2_game_poly_utils s2_game_basic_share s2_game_resource_loader
    s2_game_resource_package s2_game_transform s2_game_dg s2_game_structure
    s2_game_streams s2_game_objects s2_portable_structure
    s2_portable_package s2_game_misc_runtime s2_game_database_runtime
    -Wl,--whole-archive s2_game_dbformat_records -Wl,--no-whole-archive
    -Wl,--end-group)
  target_link_options(NativeMissionMapProbe PRIVATE -Wl,--gc-sections)
  if(S2_GAME_DB_PATH AND S2_RESOURCE_PACKAGE_PATH)
    get_filename_component(_s2_mission_res_dir "${S2_RESOURCE_PACKAGE_PATH}" DIRECTORY)
    if(EXISTS "${_s2_mission_res_dir}/Waypoints.res" AND
       EXISTS "${_s2_mission_res_dir}/Buildings.res" AND
       EXISTS "${_s2_mission_res_dir}/Terrain.res" AND
       EXISTS "${_s2_mission_res_dir}/AIGeometries.res")
      add_test(NAME NativeMissionMapProbe
        COMMAND NativeMissionMapProbe "${S2_GAME_DB_PATH}"
          "${_s2_mission_res_dir}" 218)
      if(EXISTS "${_s2_mission_res_dir}/Units.res" AND
         EXISTS "${_s2_mission_res_dir}/Groups.res")
        add_test(NAME NativeMissionRouteMapProbe
          COMMAND NativeMissionMapProbe "${S2_GAME_DB_PATH}"
            "${_s2_mission_res_dir}" 810)
        add_test(NAME NativeMissionUnitRouteMapProbe
          COMMAND NativeMissionMapProbe "${S2_GAME_DB_PATH}"
            "${_s2_mission_res_dir}" 2400)
        add_test(NAME NativeMissionLargeMapProbe
          COMMAND NativeMissionMapProbe "${S2_GAME_DB_PATH}"
            "${_s2_mission_res_dir}" 4526)
        set_tests_properties(NativeMissionLargeMapProbe PROPERTIES TIMEOUT 300)
      endif()
    endif()
  endif()
endif()
add_library(s2_game_building_grid STATIC
  "${root}/Main/BuildingGrid.cpp"
  "${root}/Main/BuildingSchema.cpp"
  "${root}/Main/RodJunction.cpp"
  "${root}/Main/rod.cpp")
target_include_directories(s2_game_building_grid PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(s2_game_building_grid PUBLIC
  s2_game_dg s2_game_transform s2_game_structure)
target_compile_features(s2_game_building_grid PUBLIC cxx_std_17)
target_compile_options(s2_game_building_grid PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_building_info STATIC "${root}/Main/BuildingInfo.cpp")
target_include_directories(s2_game_building_info PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat")
target_link_libraries(s2_game_building_info PUBLIC
  s2_game_resource_loader s2_game_dbformat_records s2_game_structure
  s2_game_basic_share)
target_compile_features(s2_game_building_info PUBLIC cxx_std_17)
target_compile_options(s2_game_building_info PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_building_internal STATIC
  "${root}/Main/MakeBuildingInternal.cpp")
target_include_directories(s2_game_building_internal PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat")
target_link_libraries(s2_game_building_internal PUBLIC
  s2_game_building_info s2_game_building_grid s2_game_dbformat_records)
target_compile_features(s2_game_building_internal PUBLIC cxx_std_17)
target_compile_options(s2_game_building_internal PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_building_clip STATIC "${root}/Main/BuildingClip.cpp")
target_include_directories(s2_game_building_clip PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(s2_game_building_clip PUBLIC
  s2_game_building_grid s2_game_structure)
target_compile_features(s2_game_building_clip PUBLIC cxx_std_17)
target_compile_options(s2_game_building_clip PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_make_building STATIC "${root}/Main/MakeBuilding.cpp")
target_include_directories(s2_game_make_building PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/ADOImport" "${root}/MiscDll")
target_link_libraries(s2_game_make_building PUBLIC
  s2_game_building_internal s2_game_building_clip
  s2_game_building_grid s2_game_building_info
  s2_game_dbformat_records s2_game_ai_geometry_loader)
target_compile_features(s2_game_make_building PUBLIC cxx_std_17)
target_compile_options(s2_game_make_building PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_map_terrain STATIC "${root}/Main/MapBuildTerrain.cpp")
target_include_directories(s2_game_map_terrain PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat")
target_link_libraries(s2_game_map_terrain PUBLIC
  s2_game_building_info s2_game_terrain_info s2_game_poly_utils
  s2_game_resource_loader s2_game_dbformat_records s2_game_basic_share)
target_compile_features(s2_game_map_terrain PUBLIC cxx_std_17)
target_compile_options(s2_game_map_terrain PRIVATE -ffunction-sections -fdata-sections)
add_executable(NativeBuildingTerrainResourceTests
  "${root}/diagnostics/NativeBuildingTerrainResourceTests.cpp")
target_link_libraries(NativeBuildingTerrainResourceTests PRIVATE
  -Wl,--start-group s2_game_make_building s2_game_building_internal
  s2_game_building_info
  s2_game_building_grid s2_game_building_clip s2_game_ai_geometry_loader
  s2_game_ai_collision s2_game_map_terrain
  s2_game_poly_utils s2_game_basic_share s2_game_resource_loader
  s2_game_resource_package s2_game_terrain_info s2_game_transform
  s2_game_dg s2_game_structure s2_game_streams s2_game_objects
  s2_portable_structure s2_portable_package s2_game_misc_runtime
  s2_game_database_runtime
  -Wl,--whole-archive s2_game_dbformat_records -Wl,--no-whole-archive
  -Wl,--end-group)
target_link_options(NativeBuildingTerrainResourceTests PRIVATE -Wl,--gc-sections)
if(S2_GAME_DB_PATH AND S2_RESOURCE_PACKAGE_PATH)
  get_filename_component(_s2_resource_dir "${S2_RESOURCE_PACKAGE_PATH}" DIRECTORY)
  if(EXISTS "${_s2_resource_dir}/Buildings.res" AND
     EXISTS "${_s2_resource_dir}/Terrain.res" AND
     EXISTS "${_s2_resource_dir}/AIGeometries.res")
    add_test(NAME NativeBuildingTerrainResourceTests
      COMMAND NativeBuildingTerrainResourceTests "${S2_GAME_DB_PATH}"
        "${_s2_resource_dir}")
  endif()
endif()
add_executable(NativeBuildingGridTests
  "${root}/diagnostics/NativeBuildingGridTests.cpp")
target_link_libraries(NativeBuildingGridTests PRIVATE
  -Wl,--start-group s2_game_building_grid s2_game_transform s2_game_dg
  s2_game_structure s2_game_streams s2_game_objects s2_portable_structure
  s2_game_misc_runtime s2_game_dbformat_records s2_game_database_runtime
  -Wl,--end-group)
target_link_options(NativeBuildingGridTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeBuildingGridTests COMMAND NativeBuildingGridTests)
add_library(s2_game_poly_utils STATIC "${root}/Main/PolyUtils.cpp")
target_include_directories(s2_game_poly_utils PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(s2_game_poly_utils PUBLIC s2_game_transform)
target_compile_features(s2_game_poly_utils PUBLIC cxx_std_17)
target_compile_options(s2_game_poly_utils PRIVATE -ffunction-sections -fdata-sections)
add_executable(NativeMapPolygonTests "${root}/diagnostics/NativeMapPolygonTests.cpp")
target_link_libraries(NativeMapPolygonTests PRIVATE s2_game_poly_utils)
target_link_options(NativeMapPolygonTests PRIVATE -Wl,--gc-sections)
add_test(NAME NativeMapPolygonTests COMMAND NativeMapPolygonTests)
add_library(s2_game_basic_share STATIC "${root}/Misc/BasicShare.cpp")
target_include_directories(s2_game_basic_share PRIVATE
  "${root}/Misc" "${root}/FileIO")
target_compile_features(s2_game_basic_share PUBLIC cxx_std_17)
add_library(s2_game_ai_waypoint STATIC "${root}/Main/aiWaypoint.cpp")
target_include_directories(s2_game_ai_waypoint PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat")
target_compile_features(s2_game_ai_waypoint PUBLIC cxx_std_17)
target_compile_options(s2_game_ai_waypoint PRIVATE -ffunction-sections -fdata-sections)
add_library(s2_game_resource_loader STATIC
  "${root}/Main/GResource.cpp"
  "${root}/Main/HeadResourceData.cpp")
target_include_directories(s2_game_resource_loader PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(s2_game_resource_loader PUBLIC
  s2_game_resource_package s2_game_dg s2_game_structure)
target_compile_features(s2_game_resource_loader PUBLIC cxx_std_17)
target_compile_options(s2_game_resource_loader PRIVATE -ffunction-sections -fdata-sections)
add_executable(NativeResourceOpenerTests
  "${root}/diagnostics/NativeResourceOpenerTests.cpp")
target_link_libraries(NativeResourceOpenerTests PRIVATE
  -Wl,--start-group s2_game_resource_loader s2_game_resource_package
  s2_game_dg s2_game_structure s2_game_streams s2_game_objects
  s2_portable_package s2_portable_structure -Wl,--end-group)
target_link_options(NativeResourceOpenerTests PRIVATE -Wl,--gc-sections)
if(S2_RESOURCE_PACKAGE_PATH)
  add_test(NAME NativeResourceOpenerTests
    COMMAND NativeResourceOpenerTests "${S2_RESOURCE_PACKAGE_PATH}"
      "${CMAKE_BINARY_DIR}/resource-opener-fixture")
endif()
add_executable(NativeWaypointResourceTests
  "${root}/diagnostics/NativeWaypointResourceTests.cpp")
target_include_directories(NativeWaypointResourceTests PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(NativeWaypointResourceTests PRIVATE
  -Wl,--start-group s2_game_ai_waypoint s2_game_basic_share
  s2_game_resource_loader s2_game_resource_package s2_game_ai_position
  s2_game_dg s2_game_structure s2_game_streams s2_game_objects
  s2_portable_package s2_portable_structure s2_game_misc_runtime
  -Wl,--end-group)
target_link_options(NativeWaypointResourceTests PRIVATE -Wl,--gc-sections)
if(S2_RESOURCE_PACKAGE_PATH)
  add_test(NAME NativeWaypointResourceTests
    COMMAND NativeWaypointResourceTests "${S2_RESOURCE_PACKAGE_PATH}")
endif()
add_executable(NativeAIRouteResourceTests
  "${root}/diagnostics/NativeAIRouteResourceTests.cpp")
target_include_directories(NativeAIRouteResourceTests PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(NativeAIRouteResourceTests PRIVATE
  -Wl,--start-group s2_game_ai_waypoint s2_game_basic_share
  s2_game_resource_loader s2_game_resource_package s2_game_ai_position
  s2_game_dg s2_game_structure s2_game_streams s2_game_objects
  s2_portable_package s2_portable_structure s2_game_misc_runtime
  s2_game_database_runtime
  -Wl,--whole-archive s2_game_dbformat_records -Wl,--no-whole-archive
  -Wl,--end-group)
target_link_options(NativeAIRouteResourceTests PRIVATE -Wl,--gc-sections)
if(S2_RESOURCE_PACKAGE_PATH AND S2_GAME_DB_PATH)
  get_filename_component(_s2_ai_route_res_dir "${S2_RESOURCE_PACKAGE_PATH}" DIRECTORY)
  if(EXISTS "${_s2_ai_route_res_dir}/Units.res" AND
     EXISTS "${_s2_ai_route_res_dir}/Groups.res")
    add_test(NAME NativeAIRouteResourceTests
      COMMAND NativeAIRouteResourceTests "${S2_GAME_DB_PATH}" "${_s2_ai_route_res_dir}")
  endif()
endif()
add_executable(NativeMapFlagsTests "${root}/diagnostics/NativeMapFlagsTests.cpp")
target_link_libraries(NativeMapFlagsTests PRIVATE
  -Wl,--start-group s2_game_map_build
  s2_game_misc_runtime s2_game_ai_grid s2_game_terrain_info
  s2_game_structure s2_game_streams s2_game_objects s2_portable_structure
  -Wl,--whole-archive s2_game_dbformat_records -Wl,--no-whole-archive
  -Wl,--end-group)
target_link_options(NativeMapFlagsTests PRIVATE -Wl,--gc-sections)
if(S2_GAME_DB_PATH)
  add_test(NAME NativeMapFlagsTests
    COMMAND NativeMapFlagsTests "${S2_GAME_DB_PATH}")
endif()

# Build the original modified Lua VM without its Windows-only save adapter.
# The runtime target is expanded as the native persistence layer is ported.
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lua_include")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/lua_include/stdafx.h"
  CONTENT "#include \"${root}/Script/StdAfx.h\"\n")
add_library(s2_game_lua STATIC
  "${root}/Script/lapi.cpp"
  "${root}/Script/lcode.cpp"
  "${root}/Script/ldebug.cpp"
  "${root}/Script/ldo.cpp"
  "${root}/Script/lfunc.cpp"
  "${root}/Script/lgc.cpp"
  "${root}/Script/llex.cpp"
  "${root}/Script/lmem.cpp"
  "${root}/Script/lobject.cpp"
  "${root}/Script/lparser.cpp"
  "${root}/Script/lsaver.cpp"
  "${root}/Script/lstate.cpp"
  "${root}/Script/lstring.cpp"
  "${root}/Script/ltable.cpp"
  "${root}/Script/ltm.cpp"
  "${root}/Script/lundump.cpp"
  "${root}/Script/lvm.cpp"
  "${root}/Script/lzio.cpp"
  "${root}/Script/Script.cpp"
  "${root}/Main/scriptRandom.cpp")
target_include_directories(s2_game_lua PRIVATE "${CMAKE_BINARY_DIR}/lua_include")
target_link_libraries(s2_game_lua PUBLIC s2_game_structure s2_game_misc_runtime)
target_compile_features(s2_game_lua PUBLIC cxx_std_17)
add_executable(NativeLuaRuntimeTests "${root}/diagnostics/NativeLuaRuntimeTests.cpp")
target_link_libraries(NativeLuaRuntimeTests PRIVATE s2_game_lua)
add_test(NAME NativeLuaRuntimeTests COMMAND NativeLuaRuntimeTests)
add_executable(NativeMissionScriptCorpusTests
  "${root}/diagnostics/NativeMissionScriptCorpusTests.cpp")
target_link_libraries(NativeMissionScriptCorpusTests PRIVATE
  -Wl,--start-group s2_game_lua
  s2_game_database_runtime s2_game_misc_runtime s2_game_structure
  s2_game_streams s2_game_objects s2_portable_structure
  -Wl,--whole-archive s2_game_dbformat_records -Wl,--no-whole-archive
  -Wl,--end-group)
if(S2_GAME_DB_PATH)
  add_test(NAME NativeMissionScriptCorpusTests
    COMMAND NativeMissionScriptCorpusTests "${S2_GAME_DB_PATH}")
endif()
s2_add_lua_corpus_test()

add_library(s2_portable_core STATIC
  "${root}/third_party/lifestudio/src/NativeCurve.cpp"
  "${root}/third_party/lifestudio/src/NativeFaceGenData.cpp"
  "${root}/third_party/lifestudio/src/NativeHeadData.cpp"
  "${root}/third_party/lifestudio/src/NativeMMTreeData.cpp"
  "${root}/third_party/lifestudio/src/NativeSequenceData.cpp")
target_include_directories(s2_portable_core PUBLIC
  "${root}/third_party/lifestudio/src"
  "${root}/third_party/lifestudio/include")
target_compile_features(s2_portable_core PUBLIC cxx_std_17)
add_library(s2_native_lifestudio STATIC
  "${root}/third_party/lifestudio/src/lifestudio_x64_stub.cpp")
target_include_directories(s2_native_lifestudio PUBLIC
  "${root}/third_party/lifestudio/include"
  "${root}/third_party/lifestudio/src")
target_compile_definitions(s2_native_lifestudio PUBLIC
  LIFESTUDIOHEADAPI_EXPORTS_LIB "__stdcall=")
target_link_libraries(s2_native_lifestudio PUBLIC s2_portable_core)
target_compile_features(s2_native_lifestudio PUBLIC cxx_std_17)
add_library(s2_game_head_runtime STATIC "${root}/Main/LSHeadPortable.cpp")
target_include_directories(s2_game_head_runtime PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc" "${root}/DBFormat"
  "${root}/third_party/lifestudio/include")
target_link_libraries(s2_game_head_runtime PUBLIC
  s2_native_lifestudio s2_game_resource_loader s2_game_basic_share
  s2_game_dbformat_records)
target_compile_features(s2_game_head_runtime PUBLIC cxx_std_17)
target_compile_options(s2_game_head_runtime PRIVATE -ffunction-sections -fdata-sections)
set(S2_HEAD_RESOURCE_PATH "" CACHE FILEPATH "Original Heads.res package for head data regression")
set(S2_HEAD_TREE_PATH "" CACHE FILEPATH "Original tree.mma for game-used facial sequence runtime")
if(S2_HEAD_RESOURCE_PATH)
  get_filename_component(_s2_head_resource_dir "${S2_HEAD_RESOURCE_PATH}" DIRECTORY)
  get_filename_component(_s2_head_game_dir "${_s2_head_resource_dir}" DIRECTORY)
  if(NOT S2_HEAD_TREE_PATH)
    set(S2_HEAD_TREE_PATH "${_s2_head_game_dir}/tree.mma")
  endif()
endif()
add_executable(NativeHeadResourceTests
  "${root}/diagnostics/NativeHeadResourceTests.cpp")
target_include_directories(NativeHeadResourceTests PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(NativeHeadResourceTests PRIVATE
  -Wl,--start-group s2_game_head_runtime s2_native_lifestudio s2_portable_core s2_game_resource_loader
  s2_game_resource_package s2_game_dg s2_game_structure
  s2_game_streams s2_game_objects s2_game_dbformat_records
  s2_game_database_runtime s2_game_misc_runtime s2_portable_database
  s2_portable_package
  s2_portable_structure -Wl,--end-group)
target_link_options(NativeHeadResourceTests PRIVATE -Wl,--gc-sections)
if(S2_HEAD_RESOURCE_PATH)
  if(S2_GAME_DB_PATH AND EXISTS "${S2_HEAD_TREE_PATH}")
    add_test(NAME NativeHeadResourceTests
      COMMAND NativeHeadResourceTests "${S2_HEAD_RESOURCE_PATH}" "${S2_HEAD_TREE_PATH}" "${S2_GAME_DB_PATH}")
  else()
    add_test(NAME NativeHeadResourceTests
      COMMAND NativeHeadResourceTests "${S2_HEAD_RESOURCE_PATH}")
  endif()
endif()
add_executable(NativeHeadSequenceRuntimeTests
  "${root}/diagnostics/NativeHeadSequenceRuntimeTests.cpp")
target_include_directories(NativeHeadSequenceRuntimeTests PRIVATE
  "${root}/Main" "${root}/FileIO" "${root}/Misc")
target_link_libraries(NativeHeadSequenceRuntimeTests PRIVATE
  -Wl,--start-group s2_game_head_runtime s2_native_lifestudio s2_portable_core s2_game_resource_loader
  s2_game_resource_package s2_game_dg s2_game_structure
  s2_game_streams s2_game_objects s2_game_dbformat_records
  s2_game_database_runtime s2_game_misc_runtime s2_portable_database
  s2_portable_package
  s2_portable_structure -Wl,--end-group)
target_link_options(NativeHeadSequenceRuntimeTests PRIVATE -Wl,--gc-sections)
if(S2_HEAD_RESOURCE_PATH)
  if(EXISTS "${_s2_head_resource_dir}/Sequences.res" AND EXISTS "${S2_HEAD_TREE_PATH}")
    add_test(NAME NativeHeadSequenceRuntimeTests
      COMMAND NativeHeadSequenceRuntimeTests "${_s2_head_resource_dir}/Sequences.res" "${S2_HEAD_TREE_PATH}")
  endif()
endif()
if(NOT WIN32)
  # These are declaration-only legacy ABI keywords; the portable data code
  # does not link or call the proprietary LifeStudio DLL.
  target_compile_definitions(s2_portable_core PRIVATE
    LIFESTUDIOHEADAPI_EXPORTS_LIB "__stdcall=")
endif()

enable_testing()
# Exercise the original AI lifecycle through the complete portable archive
# graph.  Forcing the debris visitor keeps the scene/world cast boundary in
# the link even when the lifecycle test itself does not visit a render item.
get_property(_s2_all_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
set(_s2_portable_archives)
foreach(_s2_target IN LISTS _s2_all_targets)
  get_target_property(_s2_target_type ${_s2_target} TYPE)
  if(_s2_target_type STREQUAL "STATIC_LIBRARY")
    list(APPEND _s2_portable_archives ${_s2_target})
  endif()
endforeach()
add_executable(NativeAILogicTests "${root}/diagnostics/NativeAILogicTests.cpp")
target_include_directories(NativeAILogicTests PRIVATE
  "${CMAKE_BINARY_DIR}/main_case_include" "${root}/Main")
target_link_libraries(NativeAILogicTests PRIVATE
  -Wl,--start-group ${_s2_portable_archives} -Wl,--end-group)
target_link_options(NativeAILogicTests PRIVATE -Wl,--gc-sections
  -Wl,-u,_ZN6NWorld12CDFrozenItem5VisitEPNS_14IRenderVisitorE
  -Wl,-u,_ZN6NWorld6CWorldC1EPN4NRPG11CGlobalGameE)
add_test(NAME NativeAILogicTests COMMAND NativeAILogicTests)
add_executable(NativeWorldInitProbe
  "${root}/diagnostics/NativeWorldInitProbe.cpp"
  "${root}/Main/Time.cpp")
target_include_directories(NativeWorldInitProbe PRIVATE
  "${CMAKE_BINARY_DIR}/main_case_include" "${root}/Main")
target_link_libraries(NativeWorldInitProbe PRIVATE
  -Wl,--start-group ${_s2_portable_archives} -Wl,--end-group)
target_link_options(NativeWorldInitProbe PRIVATE -Wl,--gc-sections)
if(EXISTS "${S2_GAME_DB_PATH}" AND EXISTS "${S2_RESOURCE_PACKAGE_PATH}" AND
   IS_DIRECTORY "${S2_SCRIPT_CORPUS_DIR}")
  get_filename_component(_s2_world_resources "${S2_RESOURCE_PACKAGE_PATH}" DIRECTORY)
  set(_s2_world_script_files)
  foreach(_s2_script IN ITEMS Hint.l Common.l TriggersManager.l Constants.l)
    if(EXISTS "${S2_SCRIPT_CORPUS_DIR}/${_s2_script}")
      list(APPEND _s2_world_script_files "${S2_SCRIPT_CORPUS_DIR}/${_s2_script}")
    endif()
  endforeach()
  list(LENGTH _s2_world_script_files _s2_world_script_count)
  if(_s2_world_script_count EQUAL 4)
    set(_s2_world_root "${CMAKE_BINARY_DIR}/world-probe-root")
    file(MAKE_DIRECTORY "${_s2_world_root}/Scripts")
    file(COPY ${_s2_world_script_files} DESTINATION "${_s2_world_root}/Scripts")
    add_test(NAME NativeWorldInitProbe
      COMMAND NativeWorldInitProbe "${S2_GAME_DB_PATH}" "${_s2_world_resources}")
    set_tests_properties(NativeWorldInitProbe PROPERTIES
      WORKING_DIRECTORY "${_s2_world_root}" TIMEOUT 300)
    foreach(_s2_world_variant IN ITEMS 218 810)
      add_test(NAME NativeWorldMission${_s2_world_variant}
        COMMAND NativeWorldInitProbe "${S2_GAME_DB_PATH}" "${_s2_world_resources}"
          --mission ${_s2_world_variant})
      set_tests_properties(NativeWorldMission${_s2_world_variant} PROPERTIES
        WORKING_DIRECTORY "${_s2_world_root}" TIMEOUT 300)
    endforeach()
    add_test(NAME NativeWorldMission810UIAck
      COMMAND NativeWorldInitProbe "${S2_GAME_DB_PATH}" "${_s2_world_resources}"
        --mission-ui-ack 810)
    set_tests_properties(NativeWorldMission810UIAck PROPERTIES
      WORKING_DIRECTORY "${_s2_world_root}" TIMEOUT 300)
    add_test(NAME NativeWorldMission810PartyUIAck
      COMMAND NativeWorldInitProbe "${S2_GAME_DB_PATH}" "${_s2_world_resources}"
        --mission-party-ui-ack 810)
    set_tests_properties(NativeWorldMission810PartyUIAck PROPERTIES
      WORKING_DIRECTORY "${_s2_world_root}" TIMEOUT 300)
    add_test(NAME NativeWorldMission810PartyShot
      COMMAND NativeWorldInitProbe "${S2_GAME_DB_PATH}" "${_s2_world_resources}"
        --mission-party-shot 810)
    set_tests_properties(NativeWorldMission810PartyShot PROPERTIES
      WORKING_DIRECTORY "${_s2_world_root}" TIMEOUT 300)
    add_test(NAME NativeWorldMission810PartyShotSave
      COMMAND NativeWorldInitProbe "${S2_GAME_DB_PATH}" "${_s2_world_resources}"
        --mission-party-shot-save 810 "${CMAKE_BINARY_DIR}/world-mission-810-shot.sav")
    set_tests_properties(NativeWorldMission810PartyShotSave PROPERTIES
      WORKING_DIRECTORY "${_s2_world_root}" TIMEOUT 300)
  endif()
endif()
foreach(test IN ITEMS NativeHeadDataTests NativeFaceGenDataTests
                      NativeMMTreeDataTests NativeSequenceDataTests)
  add_executable(${test} "${root}/diagnostics/${test}.cpp")
  target_link_libraries(${test} PRIVATE s2_portable_core)
  add_test(NAME ${test} COMMAND ${test})
endforeach()
