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

# Build the original modified Lua VM without its Windows-only save adapter.
# The runtime target is expanded as the native persistence layer is ported.
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/lua_include")
file(WRITE "${CMAKE_BINARY_DIR}/lua_include/stdafx.h" "#include \"${root}/Script/StdAfx.h\"\n")
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
  "${root}/Script/Script.cpp")
target_include_directories(s2_game_lua PRIVATE "${CMAKE_BINARY_DIR}/lua_include")
target_link_libraries(s2_game_lua PUBLIC s2_game_structure)
target_compile_features(s2_game_lua PUBLIC cxx_std_17)
add_executable(NativeLuaRuntimeTests "${root}/diagnostics/NativeLuaRuntimeTests.cpp")
target_link_libraries(NativeLuaRuntimeTests PRIVATE s2_game_lua)
add_test(NAME NativeLuaRuntimeTests COMMAND NativeLuaRuntimeTests)
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
if(NOT WIN32)
  # These are declaration-only legacy ABI keywords; the portable data code
  # does not link or call the proprietary LifeStudio DLL.
  target_compile_definitions(s2_portable_core PRIVATE
    LIFESTUDIOHEADAPI_EXPORTS_LIB "__stdcall=")
endif()

enable_testing()
foreach(test IN ITEMS NativeHeadDataTests NativeFaceGenDataTests
                      NativeMMTreeDataTests NativeSequenceDataTests)
  add_executable(${test} "${root}/diagnostics/${test}.cpp")
  target_link_libraries(${test} PRIVATE s2_portable_core)
  add_test(NAME ${test} COMMAND ${test})
endforeach()
