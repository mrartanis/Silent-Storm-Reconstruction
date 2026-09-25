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
target_link_libraries(s2_game_lua PUBLIC s2_game_objects)
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
