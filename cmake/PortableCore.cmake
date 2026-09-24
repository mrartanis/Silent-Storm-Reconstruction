# Platform-independent data decoding/evaluation used by the native FaceGen path.
# This is deliberately narrower than the Win32 game target; expand this list as
# other engine subsystems lose their Windows dependencies.
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
