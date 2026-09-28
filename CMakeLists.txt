# ============================================================================
#  Silent Storm engine -- CMake build
#
#  Windows x64 game; optional legacy x86 oracle; portable core on Linux/macOS
#  Configure with:            cmake -S . -B build -A x64
#  Build with:                cmake --build build --config Release
#
#  Replaces the old A5new.sln. Third-party code lives under third_party/.
# ============================================================================
cmake_minimum_required(VERSION 3.21)
project(A5 LANGUAGES C CXX)

# The game uses MSVC's historical /Gr calling convention. Keep the portable
# libraries and their probes on the same ABI when built as diagnostic x86;
# adding this only below their targets silently gives them __cdecl symbols.
if(MSVC)
  add_compile_options(/Gr)
endif()

set(root "${CMAKE_CURRENT_SOURCE_DIR}")
include("${root}/cmake/MediaProbe.cmake")
add_library(s2_portable_package STATIC "${root}/FileIO/PortablePackageIndex.cpp")
target_include_directories(s2_portable_package PUBLIC "${root}/FileIO")
target_compile_features(s2_portable_package PRIVATE cxx_std_17)
add_library(s2_portable_effects STATIC "${root}/FileIO/PortableEffectData.cpp")
target_include_directories(s2_portable_effects PUBLIC "${root}/FileIO")
target_compile_features(s2_portable_effects PRIVATE cxx_std_17)
add_executable(PortableEffectDataTests "${root}/diagnostics/PortableEffectDataTests.cpp")
target_link_libraries(PortableEffectDataTests PRIVATE s2_portable_effects)
add_test(NAME PortableEffectDataTests COMMAND PortableEffectDataTests)
add_executable(PortableEffectCorpusProbe "${root}/diagnostics/PortableEffectCorpusProbe.cpp")
target_link_libraries(PortableEffectCorpusProbe PRIVATE s2_portable_effects s2_portable_package)
target_compile_features(PortableEffectCorpusProbe PRIVATE cxx_std_17)
add_executable(PortablePackageProbe "${root}/diagnostics/PortablePackageProbe.cpp")
target_link_libraries(PortablePackageProbe PRIVATE s2_portable_package)
add_executable(PortablePackageCorpusProbe "${root}/diagnostics/PortablePackageCorpusProbe.cpp")
target_link_libraries(PortablePackageCorpusProbe PRIVATE s2_portable_package)
target_compile_features(PortablePackageCorpusProbe PRIVATE cxx_std_17)
add_executable(PortablePackageIndexTests "${root}/diagnostics/PortablePackageIndexTests.cpp")
target_link_libraries(PortablePackageIndexTests PRIVATE s2_portable_package)
target_compile_features(PortablePackageIndexTests PRIVATE cxx_std_17)
enable_testing()
set(S2_SCRIPT_CORPUS_DIR "" CACHE PATH "Directory containing the original autoload .l scripts")
function(s2_add_lua_corpus_test)
  if(NOT S2_SCRIPT_CORPUS_DIR)
    return()
  endif()
  foreach(name IN ITEMS Constants.l TriggersManager.l Common.l Hint.l)
    if(NOT EXISTS "${S2_SCRIPT_CORPUS_DIR}/${name}")
      message(FATAL_ERROR "Missing original Lua script: ${S2_SCRIPT_CORPUS_DIR}/${name}")
    endif()
  endforeach()
  add_test(NAME NativeLuaAutoloadCorpus
    COMMAND NativeLuaRuntimeTests --autoload "${S2_SCRIPT_CORPUS_DIR}")
endfunction()
add_library(s2_portable_user_paths STATIC "${root}/FileIO/PortableUserPaths.cpp")
target_include_directories(s2_portable_user_paths PUBLIC "${root}/FileIO")
target_compile_features(s2_portable_user_paths PRIVATE cxx_std_17)
add_executable(PortableUserPathsTests "${root}/diagnostics/PortableUserPathsTests.cpp")
target_link_libraries(PortableUserPathsTests PRIVATE s2_portable_user_paths)
add_test(NAME PortableUserPathsTests COMMAND PortableUserPathsTests)
add_test(NAME PortablePackageIndexTests COMMAND PortablePackageIndexTests)
add_library(s2_portable_structure STATIC "${root}/FileIO/PortableStructureChunks.cpp")
target_include_directories(s2_portable_structure PUBLIC "${root}/FileIO")
target_compile_features(s2_portable_structure PRIVATE cxx_std_17)
add_executable(PortableStructureProbe "${root}/diagnostics/PortableStructureProbe.cpp")
target_link_libraries(PortableStructureProbe PRIVATE s2_portable_structure)
add_executable(ProbeSavedExplosionPerks "${root}/diagnostics/ProbeSavedExplosionPerks.cpp")
target_link_libraries(ProbeSavedExplosionPerks PRIVATE s2_portable_structure)
add_executable(ProbeSavedBuildingVoxels "${root}/diagnostics/ProbeSavedBuildingVoxels.cpp")
target_link_libraries(ProbeSavedBuildingVoxels PRIVATE s2_portable_structure)
add_executable(ProbeSavedRuntimeType "${root}/diagnostics/ProbeSavedRuntimeType.cpp")
target_link_libraries(ProbeSavedRuntimeType PRIVATE s2_portable_structure)
add_executable(PortableStructureChunkTests "${root}/diagnostics/PortableStructureChunkTests.cpp")
target_link_libraries(PortableStructureChunkTests PRIVATE s2_portable_structure)
add_test(NAME PortableStructureChunkTests COMMAND PortableStructureChunkTests)
add_executable(PortableCameraWireTests "${root}/diagnostics/PortableCameraWireTests.cpp")
add_test(NAME PortableCameraWireTests COMMAND PortableCameraWireTests)
add_executable(PortableVertexWireTests "${root}/diagnostics/PortableVertexWireTests.cpp")
add_test(NAME PortableVertexWireTests COMMAND PortableVertexWireTests)
add_executable(PortableGeometryWireTests "${root}/diagnostics/PortableGeometryWireTests.cpp")
add_test(NAME PortableGeometryWireTests COMMAND PortableGeometryWireTests)
add_executable(PortableFBTransformWireTests "${root}/diagnostics/PortableFBTransformWireTests.cpp")
add_test(NAME PortableFBTransformWireTests COMMAND PortableFBTransformWireTests)
add_executable(PortableBoolSyncWireTests "${root}/diagnostics/PortableBoolSyncWireTests.cpp")
add_test(NAME PortableBoolSyncWireTests COMMAND PortableBoolSyncWireTests)
add_executable(PortableConvexHullMapWireTests "${root}/diagnostics/PortableConvexHullMapWireTests.cpp")
add_test(NAME PortableConvexHullMapWireTests COMMAND PortableConvexHullMapWireTests)
add_executable(PortableFontWireTests "${root}/diagnostics/PortableFontWireTests.cpp")
add_test(NAME PortableFontWireTests COMMAND PortableFontWireTests)
add_executable(PortableAITileWireTests "${root}/diagnostics/PortableAITileWireTests.cpp")
add_test(NAME PortableAITileWireTests COMMAND PortableAITileWireTests)
add_executable(PortableVisionQueryWireTests "${root}/diagnostics/PortableVisionQueryWireTests.cpp")
add_test(NAME PortableVisionQueryWireTests COMMAND PortableVisionQueryWireTests)
add_executable(PortableVectorHashTests "${root}/diagnostics/PortableVectorHashTests.cpp")
add_test(NAME PortableVectorHashTests COMMAND PortableVectorHashTests)
add_executable(PortableMeshWireTests "${root}/diagnostics/PortableMeshWireTests.cpp")
add_test(NAME PortableMeshWireTests COMMAND PortableMeshWireTests)
add_executable(PortableAIColourWireTests "${root}/diagnostics/PortableAIColourWireTests.cpp")
add_test(NAME PortableAIColourWireTests COMMAND PortableAIColourWireTests)
add_executable(PortableGroupWireTests "${root}/diagnostics/PortableGroupWireTests.cpp")
add_test(NAME PortableGroupWireTests COMMAND PortableGroupWireTests)
add_executable(PortableResourceKeyWireTests "${root}/diagnostics/PortableResourceKeyWireTests.cpp")
add_test(NAME PortableResourceKeyWireTests COMMAND PortableResourceKeyWireTests)
add_executable(PortableClipShareWireTests "${root}/diagnostics/PortableClipShareWireTests.cpp")
add_test(NAME PortableClipShareWireTests COMMAND PortableClipShareWireTests)
add_executable(PortablePartFlagsWireTests "${root}/diagnostics/PortablePartFlagsWireTests.cpp")
add_test(NAME PortablePartFlagsWireTests COMMAND PortablePartFlagsWireTests)
add_executable(PortableBuildingPartTests "${root}/diagnostics/PortableBuildingPartTests.cpp")
add_test(NAME PortableBuildingPartTests COMMAND PortableBuildingPartTests)
add_executable(PortableSquareMoveWireTests "${root}/diagnostics/PortableSquareMoveWireTests.cpp")
add_test(NAME PortableSquareMoveWireTests COMMAND PortableSquareMoveWireTests)
add_executable(PortableMapStoreyWireTests "${root}/diagnostics/PortableMapStoreyWireTests.cpp")
add_test(NAME PortableMapStoreyWireTests COMMAND PortableMapStoreyWireTests)
add_executable(PortableAnimationWireTests "${root}/diagnostics/PortableAnimationWireTests.cpp")
add_test(NAME PortableAnimationWireTests COMMAND PortableAnimationWireTests)
add_executable(PortableCameraEffectWireTests "${root}/diagnostics/PortableCameraEffectWireTests.cpp")
add_test(NAME PortableCameraEffectWireTests COMMAND PortableCameraEffectWireTests)
add_executable(PortableLuaStateWireTests "${root}/diagnostics/PortableLuaStateWireTests.cpp")
add_test(NAME PortableLuaStateWireTests COMMAND PortableLuaStateWireTests)
add_executable(PortableSelectionInfoWireTests "${root}/diagnostics/PortableSelectionInfoWireTests.cpp")
add_test(NAME PortableSelectionInfoWireTests COMMAND PortableSelectionInfoWireTests)
add_executable(PortableRenderStateWireTests "${root}/diagnostics/PortableRenderStateWireTests.cpp")
add_test(NAME PortableRenderStateWireTests COMMAND PortableRenderStateWireTests)
add_executable(PortableLightKeyWireTests "${root}/diagnostics/PortableLightKeyWireTests.cpp")
add_test(NAME PortableLightKeyWireTests COMMAND PortableLightKeyWireTests)
add_executable(PortableAIDistanceWireTests "${root}/diagnostics/PortableAIDistanceWireTests.cpp")
add_test(NAME PortableAIDistanceWireTests COMMAND PortableAIDistanceWireTests)
add_executable(PortablePixel8888WireTests "${root}/diagnostics/PortablePixel8888WireTests.cpp")
add_test(NAME PortablePixel8888WireTests COMMAND PortablePixel8888WireTests)
add_executable(PortableSaveHeaderTests "${root}/diagnostics/PortableSaveHeaderTests.cpp")
add_test(NAME PortableSaveHeaderTests COMMAND PortableSaveHeaderTests)
add_executable(PortableScenarioSignatureTests
  "${root}/diagnostics/PortableScenarioSignatureTests.cpp")
target_compile_features(PortableScenarioSignatureTests PRIVATE cxx_std_17)
add_test(NAME PortableScenarioSignatureTests COMMAND PortableScenarioSignatureTests)
add_executable(PortableSaveHeaderProbe "${root}/diagnostics/PortableSaveHeaderProbe.cpp")
add_executable(PortableExplosionPerkWireTests "${root}/diagnostics/PortableExplosionPerkWireTests.cpp")
add_test(NAME PortableExplosionPerkWireTests COMMAND PortableExplosionPerkWireTests)
add_executable(PortableParticleWireTests "${root}/diagnostics/PortableParticleWireTests.cpp")
add_test(NAME PortableParticleWireTests COMMAND PortableParticleWireTests)
add_executable(PortableRandTests "${root}/diagnostics/PortableRandTests.cpp")
add_test(NAME PortableRandTests COMMAND PortableRandTests)
add_executable(PortableFloat2IntTests "${root}/diagnostics/PortableFloat2IntTests.cpp")
target_compile_features(PortableFloat2IntTests PRIVATE cxx_std_17)
add_test(NAME PortableFloat2IntTests COMMAND PortableFloat2IntTests)
add_executable(PortablePathPlaceTests "${root}/diagnostics/PortablePathPlaceTests.cpp")
target_compile_features(PortablePathPlaceTests PRIVATE cxx_std_17)
add_test(NAME PortablePathPlaceTests COMMAND PortablePathPlaceTests)
add_executable(PortableZoneWireTests "${root}/diagnostics/PortableZoneWireTests.cpp")
target_compile_features(PortableZoneWireTests PRIVATE cxx_std_17)
add_test(NAME PortableZoneWireTests COMMAND PortableZoneWireTests)
add_executable(PortableObjectPlaceTests "${root}/diagnostics/PortableObjectPlaceTests.cpp")
target_compile_features(PortableObjectPlaceTests PRIVATE cxx_std_17)
add_test(NAME PortableObjectPlaceTests COMMAND PortableObjectPlaceTests)
add_executable(PortableCriticalTests "${root}/diagnostics/PortableCriticalTests.cpp")
target_compile_features(PortableCriticalTests PRIVATE cxx_std_17)
add_test(NAME PortableCriticalTests COMMAND PortableCriticalTests)
add_executable(PortableJunctionTests "${root}/diagnostics/PortableJunctionTests.cpp")
target_compile_features(PortableJunctionTests PRIVATE cxx_std_17)
add_test(NAME PortableJunctionTests COMMAND PortableJunctionTests)
add_executable(PortableIsaacTests "${root}/diagnostics/PortableIsaacTests.cpp")
add_test(NAME PortableIsaacTests COMMAND PortableIsaacTests)
add_executable(ProbeSavedDecalVertices "${root}/diagnostics/ProbeSavedDecalVertices.cpp")
target_link_libraries(ProbeSavedDecalVertices PRIVATE s2_portable_structure)
add_library(s2_portable_database STATIC
  "${root}/FileIO/PortableGameDatabase.cpp"
  "${root}/FileIO/PortableGameLinks.cpp")
target_include_directories(s2_portable_database PUBLIC "${root}/FileIO")
target_link_libraries(s2_portable_database PUBLIC s2_portable_structure)
target_compile_features(s2_portable_database PRIVATE cxx_std_17)
add_executable(PortableGameDatabaseProbe "${root}/diagnostics/PortableGameDatabaseProbe.cpp")
target_link_libraries(PortableGameDatabaseProbe PRIVATE s2_portable_database)
add_executable(PortableGameLinksTests "${root}/diagnostics/PortableGameLinksTests.cpp")
target_link_libraries(PortableGameLinksTests PRIVATE s2_portable_database)
add_test(NAME PortableGameLinksTests COMMAND PortableGameLinksTests)
add_executable(NativeLuaWordTests "${root}/diagnostics/NativeLuaWordTests.cpp")
add_test(NAME NativeLuaWordTests COMMAND NativeLuaWordTests)
add_executable(PortableVoxelObjectHashTests
  "${root}/diagnostics/PortableVoxelObjectHashTests.cpp")
target_compile_features(PortableVoxelObjectHashTests PRIVATE cxx_std_17)
add_test(NAME PortableVoxelObjectHashTests COMMAND PortableVoxelObjectHashTests)
if(NOT MSVC)
  include("${root}/cmake/PortableCore.cmake")
  return()
endif()
add_executable(NativeRandParityTests "${root}/diagnostics/NativeRandParityTests.cpp")
add_test(NAME NativeRandParityTests COMMAND NativeRandParityTests)
add_executable(NativeIsaacParityTests "${root}/diagnostics/NativeIsaacParityTests.cpp")
add_test(NAME NativeIsaacParityTests COMMAND NativeIsaacParityTests)
add_executable(WindowsSaveNameTests
  "${root}/diagnostics/WindowsSaveNameTests.cpp"
  "${root}/FileIO/WindowsSaveNames.cpp")
add_test(NAME WindowsSaveNameTests COMMAND WindowsSaveNameTests)
enable_language(RC)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Unsupported pointer size: ${CMAKE_SIZEOF_VOID_P}")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(S2_MSVC_MACHINE x64)
else()
  set(S2_MSVC_MACHINE x86)
endif()

set(CMAKE_CXX_STANDARD 14)

# Dynamic CRT (/MD, /MDd) -- matches the original projects and the committed import libs.
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Engine-wide compile settings: __fastcall default (/Gr), big translation units,
# C++ exceptions, parallel compile, and the common WIN32 defines. externA5 is
# libpng's API-decoration macro -- 'extern' here (we build libpng statically).
add_compile_options(/Zm300 /FS $<$<COMPILE_LANGUAGE:CXX>:/EHsc> /wd4996)  # no /MP: ADOImport uses #import
add_compile_definitions(
  WIN32 _WINDOWS externA5=extern
  $<$<CONFIG:Debug>:_DEBUG FAST_DEBUG>
  $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)

# The engine's .rc files #include "afxres.h" (an MFC header), but the engine isn't
# MFC; redirect it to the SDK's non-MFC winres.h via a generated shim on the RC path.
file(WRITE "${CMAKE_BINARY_DIR}/rc_compat/afxres.h" "#include <winres.h>\n")

# ----------------------------------------------------------------------------
#  third_party
# ----------------------------------------------------------------------------
# zlib -- SHARED: the engine's zconf.h defaults consumers to __declspec(dllimport)
# (zlib is expected to be a DLL), so build one. ZLIB_DLL (this target only) = export;
# consumers see dllimport and resolve against the generated import lib.
file(GLOB zlib_src "${root}/third_party/zlib/*.c")
add_library(zlib SHARED ${zlib_src})
target_include_directories(zlib PUBLIC "${root}/third_party/zlib")
target_compile_definitions(zlib PRIVATE ZLIB_DLL _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)

# libpng (static; the MMX-asm variants are dropped in favour of the C path)
file(GLOB png_src "${root}/third_party/libpng/*.c")
list(FILTER png_src EXCLUDE REGEX "(pnggccrd|pngvcrd)\\.c$")
add_library(libpng STATIC ${png_src})
target_include_directories(libpng PUBLIC "${root}/third_party/libpng")
target_link_libraries(libpng PUBLIC zlib)
target_compile_definitions(libpng PRIVATE _CRT_SECURE_NO_WARNINGS PNG_NO_MMX_CODE)

# libsquish (DXT compression) -- built from source
file(GLOB squish_src "${root}/third_party/squish/squish/*.cpp")
add_library(squish STATIC ${squish_src})
target_include_directories(squish PUBLIC "${root}/third_party/squish" PRIVATE "${root}/third_party/squish/squish")

# Proprietary-DLL interop: the import libs are GENERATED at build time from the
# committed .def export tables.
set(_implib_dir "${CMAKE_BINARY_DIR}/_implibs")
get_filename_component(_vcbin "${CMAKE_C_COMPILER}" DIRECTORY)

# fmod/bink export decorated __stdcall names (_Name@bytes) that `lib /def` can't
# reproduce, so compile a tiny stub DLL whose dllexport __stdcall stubs carry the same
# names and keep the import lib the linker emits. The stub DLL is routed to a side dir
# (NEVER next to Game.exe) so it can never shadow the real runtime DLL.
function(s2_stub_implib target dllname deffile)
  file(STRINGS "${deffile}" _ex REGEX "_[A-Za-z0-9_]+@[0-9]+")
  set(_c "/* generated -- mints the import lib for ${dllname}.dll (do not edit) */\n")
  foreach(_e IN LISTS _ex)
    if(_e MATCHES "_([A-Za-z0-9_]+)@([0-9]+)")
      set(_nm "${CMAKE_MATCH_1}")
      math(EXPR _n "${CMAKE_MATCH_2} / 4")
      set(_p "void")
      if(_n GREATER 0)
        set(_p "")
        foreach(_i RANGE 1 ${_n})
          string(APPEND _p "int a${_i},")
        endforeach()
        string(REGEX REPLACE ",$" "" _p "${_p}")
      endif()
      string(APPEND _c "__declspec(dllexport) int __stdcall ${_nm}(${_p}){return 0;}\n")
    endif()
  endforeach()
  set(_src "${CMAKE_BINARY_DIR}/_implib_gen/${target}.c")
  file(WRITE "${_src}" "${_c}")
  add_library(${target} SHARED "${_src}")
  set_target_properties(${target} PROPERTIES OUTPUT_NAME "${dllname}")
  foreach(_cfg "" _DEBUG _RELEASE _RELWITHDEBINFO _MINSIZEREL)
    set_target_properties(${target} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY${_cfg} "${_implib_dir}"
      ARCHIVE_OUTPUT_DIRECTORY${_cfg} "${_implib_dir}")
  endforeach()
endfunction()

s2_stub_implib(fmod_stub fmod    "${root}/third_party/fmod/def/fmod.def")
s2_stub_implib(bink_stub binkw32 "${root}/third_party/bink/def/bink.def")

# LifeStudio exports C++ mangled names, which `lib /def` reproduces directly.
set(_ls_implibs "")
foreach(_d LifeStudioHeadAPI GDPFile)
  add_custom_command(OUTPUT "${_implib_dir}/${_d}.lib"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${_implib_dir}"
    COMMAND "${_vcbin}/lib.exe" /nologo "/def:${root}/third_party/lifestudio/def/${_d}.def"
            "/out:${_implib_dir}/${_d}.lib" "/machine:${S2_MSVC_MACHINE}"
    DEPENDS "${root}/third_party/lifestudio/def/${_d}.def" VERBATIM
    COMMENT "Generating import lib ${_d}.lib")
  list(APPEND _ls_implibs "${_implib_dir}/${_d}.lib")
endforeach()
add_custom_target(ls_implibs DEPENDS ${_ls_implibs})

# LifeStudio Init/Compute glue is real source -> a normal static lib.
add_library(lifestudio_init STATIC "${root}/third_party/lifestudio/src/lsglue.cpp")
target_include_directories(lifestudio_init PRIVATE "${root}/third_party/lifestudio/include")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # The retained LifeStudio import libraries encode x86 C++ symbol names.
    # Use the partial native x64 bridge while bone/muscle evaluation is ported.
  target_sources(lifestudio_init PRIVATE
    "${root}/third_party/lifestudio/src/lifestudio_x64_stub.cpp"
    "${root}/third_party/lifestudio/src/NativeGDP.cpp")
  target_compile_definitions(lifestudio_init PUBLIC LIFESTUDIOHEADAPI_EXPORTS_LIB)
  target_link_libraries(lifestudio_init PUBLIC ole32)
endif()

# The three interop shims: clean-room include dir + the generated import libs.
add_library(fmod_shim INTERFACE)
target_include_directories(fmod_shim INTERFACE "${root}/third_party/fmod/include")
option(S2_ENABLE_NATIVE_SFX "Use FFmpeg/miniaudio for x64 effects and voices" OFF)
if(NOT S2_ENABLE_NATIVE_SFX)
  target_link_libraries(fmod_shim INTERFACE fmod_stub)
endif()

add_library(bink_shim INTERFACE)
target_include_directories(bink_shim INTERFACE "${root}/third_party/bink/include")
target_link_libraries(bink_shim INTERFACE bink_stub)          # GBinkPlayer.cpp #pragma resolves via this import lib
target_link_directories(bink_shim INTERFACE "${_implib_dir}") # ...and its #pragma comment(lib,"binkw32.lib") by name

add_library(lifestudio_shim INTERFACE)
target_include_directories(lifestudio_shim INTERFACE "${root}/third_party/lifestudio/include")
target_link_libraries(lifestudio_shim INTERFACE lifestudio_init)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  target_link_libraries(lifestudio_shim INTERFACE
    "${_implib_dir}/LifeStudioHeadAPI.lib" "${_implib_dir}/GDPFile.lib")
endif()

# A headless x86-oracle / x64-native comparison tool. Inputs are original
# LifeStudio streams exported into an ignored lab directory, never committed.
add_executable(FaceProbe "${root}/diagnostics/FaceProbe.cpp")
target_link_libraries(FaceProbe PRIVATE lifestudio_shim)
add_executable(FaceGDPProbe "${root}/diagnostics/FaceGDPProbe.cpp")
target_link_libraries(FaceGDPProbe PRIVATE lifestudio_shim)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  target_include_directories(FaceProbe PRIVATE "${root}/third_party/lifestudio/src")
endif()
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  add_dependencies(FaceProbe ls_implibs)
  add_dependencies(FaceGDPProbe ls_implibs)
endif()

  add_library(lifestudio_native_data STATIC
    "${root}/third_party/lifestudio/src/NativeCurve.cpp"
    "${root}/third_party/lifestudio/src/NativeFaceGenData.cpp"
    "${root}/third_party/lifestudio/src/NativeHeadData.cpp"
    "${root}/third_party/lifestudio/src/NativeMMTreeData.cpp"
    "${root}/third_party/lifestudio/src/NativeSequenceData.cpp")
  target_include_directories(lifestudio_native_data PUBLIC
    "${root}/third_party/lifestudio/src")
  target_include_directories(lifestudio_native_data PRIVATE
    "${root}/third_party/lifestudio/include")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_link_libraries(lifestudio_init PUBLIC lifestudio_native_data)
    add_executable(NativeMMTreeApiCheck "${root}/diagnostics/NativeMMTreeApiCheck.cpp")
    target_link_libraries(NativeMMTreeApiCheck PRIVATE lifestudio_shim lifestudio_native_data)
    add_executable(NativeGDPApiCheck "${root}/diagnostics/NativeGDPApiCheck.cpp")
    target_link_libraries(NativeGDPApiCheck PRIVATE lifestudio_shim)
    add_executable(NativeFaceGenApiCheck "${root}/diagnostics/NativeFaceGenApiCheck.cpp")
    target_link_libraries(NativeFaceGenApiCheck PRIVATE lifestudio_shim lifestudio_native_data)
    add_executable(NativeFaceGenBlendCheck "${root}/diagnostics/NativeFaceGenBlendCheck.cpp")
    target_link_libraries(NativeFaceGenBlendCheck PRIVATE lifestudio_shim lifestudio_native_data)
    add_executable(NativeFaceGenSelectorParamCheck "${root}/diagnostics/NativeFaceGenSelectorParamCheck.cpp")
    target_link_libraries(NativeFaceGenSelectorParamCheck PRIVATE lifestudio_native_data)
    add_executable(NativeFaceGenGameWeightCheck "${root}/diagnostics/NativeFaceGenGameWeightCheck.cpp")
    target_link_libraries(NativeFaceGenGameWeightCheck PRIVATE lifestudio_native_data)
    add_executable(NativeFaceGenOutputTransferCheck "${root}/diagnostics/NativeFaceGenOutputTransferCheck.cpp")
    target_link_libraries(NativeFaceGenOutputTransferCheck PRIVATE lifestudio_native_data)
  endif()
  add_executable(NativeHeadDecode "${root}/diagnostics/NativeHeadDecode.cpp")
  target_link_libraries(NativeHeadDecode PRIVATE lifestudio_native_data)
  add_executable(GDPStorageProbe "${root}/diagnostics/GDPStorageProbe.cpp")
  target_link_libraries(GDPStorageProbe PRIVATE ole32)
  target_link_options(GDPStorageProbe PRIVATE /SUBSYSTEM:CONSOLE /ENTRY:wmainCRTStartup)
  add_executable(NativeMMTreeDecode "${root}/diagnostics/NativeMMTreeDecode.cpp")
  target_link_libraries(NativeMMTreeDecode PRIVATE lifestudio_native_data)
  add_executable(NativeMMTreeCurveEvaluate "${root}/diagnostics/NativeMMTreeCurveEvaluate.cpp")
  target_link_libraries(NativeMMTreeCurveEvaluate PRIVATE lifestudio_native_data)
  add_executable(NativeMMTreeMacroEvaluate "${root}/diagnostics/NativeMMTreeMacroEvaluate.cpp")
  target_link_libraries(NativeMMTreeMacroEvaluate PRIVATE lifestudio_native_data)
  add_executable(NativeMMTreeFilterEffects "${root}/diagnostics/NativeMMTreeFilterEffects.cpp")
  target_link_libraries(NativeMMTreeFilterEffects PRIVATE lifestudio_native_data)
  target_compile_features(NativeMMTreeFilterEffects PRIVATE cxx_std_17)
  add_executable(NativeSequenceDecode "${root}/diagnostics/NativeSequenceDecode.cpp")
  target_link_libraries(NativeSequenceDecode PRIVATE lifestudio_native_data)
  add_executable(NativeSequenceEvaluate "${root}/diagnostics/NativeSequenceEvaluate.cpp")
  target_link_libraries(NativeSequenceEvaluate PRIVATE lifestudio_native_data)
enable_testing()
add_executable(NativeHeadSeedTests "${root}/diagnostics/NativeHeadSeedTests.cpp")
target_compile_features(NativeHeadSeedTests PRIVATE cxx_std_17)
add_test(NAME NativeHeadSeedTests COMMAND NativeHeadSeedTests)
add_executable(NativeHeadDataTests "${root}/diagnostics/NativeHeadDataTests.cpp")
target_link_libraries(NativeHeadDataTests PRIVATE lifestudio_native_data)
  add_test(NAME NativeHeadDataTests COMMAND NativeHeadDataTests)
  add_executable(NativeFaceGenDataTests "${root}/diagnostics/NativeFaceGenDataTests.cpp")
  target_link_libraries(NativeFaceGenDataTests PRIVATE lifestudio_native_data)
  add_test(NAME NativeFaceGenDataTests COMMAND NativeFaceGenDataTests)
  add_executable(NativeMMTreeDataTests "${root}/diagnostics/NativeMMTreeDataTests.cpp")
  target_link_libraries(NativeMMTreeDataTests PRIVATE lifestudio_native_data)
  add_test(NAME NativeMMTreeDataTests COMMAND NativeMMTreeDataTests)
  add_executable(NativeSequenceDataTests "${root}/diagnostics/NativeSequenceDataTests.cpp")
  target_link_libraries(NativeSequenceDataTests PRIVATE lifestudio_native_data)
  add_test(NAME NativeSequenceDataTests COMMAND NativeSequenceDataTests)

# ----------------------------------------------------------------------------
#  engine modules  (each is a directory of *.cpp + a StdAfx.h PCH)
# ----------------------------------------------------------------------------
# Per-project source lists, baked from the original .vcxproj manifests (the projects
# compiled an explicit list; several dirs also hold dead alternative .cpp, so we never
# glob). Defines <Project>_SRC, paths relative to the project directory.
include("${root}/sources.cmake")

function(s2_module name type)
  cmake_parse_arguments(M "" "" "LINK;DEFINES;WHOLE" ${ARGN})
  set(src "")
  foreach(_rel IN LISTS ${name}_SRC)
    get_filename_component(_abs "${root}/${name}/${_rel}" ABSOLUTE)
    list(APPEND src "${_abs}")
  endforeach()
  if(NOT src)
    message(FATAL_ERROR "s2_module(${name}): no sources -- missing from sources.cmake?")
  endif()
  if(type STREQUAL "lib")
    add_library(${name} STATIC ${src})
  elseif(type STREQUAL "win32")
    add_executable(${name} WIN32 ${src})
  else()
    add_executable(${name} ${src})
  endif()
  target_include_directories(${name} PRIVATE "${CMAKE_BINARY_DIR}/rc_compat")  # afxres.h -> winres.h
  if(EXISTS "${root}/${name}/StdAfx.h")
    target_precompile_headers(${name} PRIVATE "${root}/${name}/StdAfx.h")
  endif()
  if(M_LINK)
    target_link_libraries(${name} PUBLIC ${M_LINK})
  endif()
  # Whole-archive: pull in ALL objects of these static libs even if no symbol is
  # directly referenced -- the engine self-registers classes via global-ctor side
  # effects (BASIC_REGISTER_CLASS / REGISTER_SAVELOAD_CLASS), which the linker would
  # otherwise discard. (Matches the VS UseLibraryDependencyInputs=true on Main.)
  if(M_WHOLE)
    list(JOIN M_WHOLE "," _wa)
    target_link_libraries(${name} PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,${_wa}>")
  endif()
  target_compile_definitions(${name} PRIVATE $<$<STREQUAL:${type},lib>:_USRDLL> ${M_DEFINES})
endfunction()

# leaves
s2_module(Misc        lib)
add_executable(NativeObjectCoreTests "${root}/diagnostics/NativeObjectCoreTests.cpp")
target_link_libraries(NativeObjectCoreTests PRIVATE Misc)
add_test(NAME NativeObjectCoreTests COMMAND NativeObjectCoreTests)
# (Scintilla + OpenDynamix are only used by MapEdit, which isn't built -- omitted.)
# core
s2_module(FileIO    lib LINK Misc s2_portable_package s2_portable_structure s2_portable_user_paths)
add_executable(PackageOracleProbe "${root}/diagnostics/PackageOracleProbe.cpp")
target_link_libraries(PackageOracleProbe PRIVATE FileIO Misc)
add_executable(StructureWireProbe "${root}/diagnostics/StructureWireProbe.cpp")
target_link_libraries(StructureWireProbe PRIVATE FileIO Misc)
add_test(NAME StructureWireWrite
  COMMAND StructureWireProbe write "${CMAKE_BINARY_DIR}/StructureWireProbe.bin")
add_test(NAME StructureWireRead
  COMMAND StructureWireProbe read "${CMAKE_BINARY_DIR}/StructureWireProbe.bin")
set_tests_properties(StructureWireWrite PROPERTIES FIXTURES_SETUP structure_wire)
set_tests_properties(StructureWireRead PROPERTIES FIXTURES_REQUIRED structure_wire)
add_executable(NativeCruncherTests "${root}/diagnostics/NativeCruncherTests.cpp")
target_link_libraries(NativeCruncherTests PRIVATE FileIO)
add_test(NAME NativeCruncherTests COMMAND NativeCruncherTests)
add_executable(NativeResourcePackageTests
  "${root}/diagnostics/NativeResourcePackageTests.cpp")
target_link_libraries(NativeResourcePackageTests PRIVATE FileIO)
target_compile_features(NativeResourcePackageTests PRIVATE cxx_std_17)
s2_module(MiscDll   lib LINK FileIO Misc DEFINES MISCDLL_EXPORTS)
s2_module(Image     lib LINK zlib libpng squish)
s2_module(ADOImport lib LINK FileIO Misc DEFINES ADOIMPORT_EXPORTS)
target_link_libraries(ADOImport PRIVATE s2_portable_database)
s2_module(FModSound lib LINK FileIO Misc fmod_shim)
s2_module(Input     lib LINK FileIO MiscDll Misc dxguid dinput8)
s2_module(Script    lib LINK FileIO Input MiscDll Misc)
add_executable(NativeStreamsTests "${root}/diagnostics/NativeStreamsTests.cpp")
target_link_libraries(NativeStreamsTests PRIVATE FileIO)
target_compile_features(NativeStreamsTests PRIVATE cxx_std_17)
add_test(NAME NativeStreamsTests COMMAND NativeStreamsTests)
add_executable(NativeLuaRuntimeTests
  "${root}/diagnostics/NativeLuaRuntimeTests.cpp"
  "${root}/Main/scriptRandom.cpp")
target_link_libraries(NativeLuaRuntimeTests PRIVATE Script)
add_test(NAME NativeLuaRuntimeTests COMMAND NativeLuaRuntimeTests)
s2_add_lua_corpus_test()
s2_module(DBFormat  lib LINK ADOImport FileIO Misc)
s2_module(Main      lib LINK ADOImport DBFormat FileIO FModSound Input MiscDll Misc Script
                            lifestudio_shim bink_shim d3d9 dxguid s2_portable_user_paths
                            s2_portable_effects)
add_executable(NativeCombinerCoreTests
  "${root}/diagnostics/NativeCombinerCoreTests.cpp")
target_include_directories(NativeCombinerCoreTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeCombinerCoreTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeCombinerCoreTests PRIVATE Main)
add_test(NAME NativeCombinerCoreTests COMMAND NativeCombinerCoreTests)
add_executable(NativeScenePartCoreTests
  "${root}/diagnostics/NativeScenePartCoreTests.cpp"
  "${root}/Main/GScenePartCore.cpp")
target_include_directories(NativeScenePartCoreTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeScenePartCoreTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeScenePartCoreTests PRIVATE Main)
add_test(NAME NativeScenePartCoreTests COMMAND NativeScenePartCoreTests)
add_executable(NativeVoxelWireTests "${root}/diagnostics/NativeVoxelWireTests.cpp")
target_include_directories(NativeVoxelWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeVoxelWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeVoxelWireTests COMMAND NativeVoxelWireTests)
add_executable(NativeCameraWireTests "${root}/diagnostics/NativeCameraWireTests.cpp")
target_include_directories(NativeCameraWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeCameraWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeCameraWireTests COMMAND NativeCameraWireTests)
add_executable(NativePathPlaceParityTests "${root}/diagnostics/NativePathPlaceParityTests.cpp")
target_include_directories(NativePathPlaceParityTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativePathPlaceParityTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativePathPlaceParityTests COMMAND NativePathPlaceParityTests)
add_executable(NativeZoneWireTests "${root}/diagnostics/NativeZoneWireTests.cpp")
target_include_directories(NativeZoneWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeZoneWireTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeZoneWireTests PRIVATE Main)
add_test(NAME NativeZoneWireTests COMMAND NativeZoneWireTests)
add_executable(NativeObjectPlaceTests "${root}/diagnostics/NativeObjectPlaceTests.cpp")
target_include_directories(NativeObjectPlaceTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeObjectPlaceTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeObjectPlaceTests PRIVATE Main)
add_test(NAME NativeObjectPlaceTests COMMAND NativeObjectPlaceTests)
add_executable(NativeCriticalTests "${root}/diagnostics/NativeCriticalTests.cpp")
target_include_directories(NativeCriticalTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeCriticalTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeCriticalTests PRIVATE Main)
add_test(NAME NativeCriticalTests COMMAND NativeCriticalTests)
add_executable(NativeJunctionTests "${root}/diagnostics/NativeJunctionTests.cpp")
target_include_directories(NativeJunctionTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeJunctionTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeJunctionTests PRIVATE Main)
add_test(NAME NativeJunctionTests COMMAND NativeJunctionTests)
add_executable(NativeVertexWireTests "${root}/diagnostics/NativeVertexWireTests.cpp")
target_include_directories(NativeVertexWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeVertexWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeVertexWireTests COMMAND NativeVertexWireTests)
add_executable(NativeGeometryWireTests "${root}/diagnostics/NativeGeometryWireTests.cpp")
target_include_directories(NativeGeometryWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeGeometryWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeGeometryWireTests COMMAND NativeGeometryWireTests)
add_executable(NativeTransformTests "${root}/diagnostics/NativeTransformTests.cpp")
target_include_directories(NativeTransformTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeTransformTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeTransformTests PRIVATE Main)
add_test(NAME NativeTransformTests COMMAND NativeTransformTests)
add_executable(NativeBetaSplineTests "${root}/diagnostics/NativeBetaSplineTests.cpp")
target_include_directories(NativeBetaSplineTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeBetaSplineTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeBetaSplineTests PRIVATE Main)
add_test(NAME NativeBetaSplineTests COMMAND NativeBetaSplineTests)
add_executable(NativeDGTests "${root}/diagnostics/NativeDGTests.cpp")
target_include_directories(NativeDGTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeDGTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeDGTests PRIVATE Main)
add_test(NAME NativeDGTests COMMAND NativeDGTests)
add_executable(NativeTerrainInfoTests "${root}/diagnostics/NativeTerrainInfoTests.cpp")
target_include_directories(NativeTerrainInfoTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeTerrainInfoTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeTerrainInfoTests PRIVATE Main)
add_test(NAME NativeTerrainInfoTests COMMAND NativeTerrainInfoTests)
add_executable(NativeHeightLayersTests "${root}/diagnostics/NativeHeightLayersTests.cpp")
target_include_directories(NativeHeightLayersTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeHeightLayersTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeHeightLayersTests PRIVATE Main)
add_test(NAME NativeHeightLayersTests COMMAND NativeHeightLayersTests)
add_executable(NativeHeightNetworkTests "${root}/diagnostics/NativeHeightNetworkTests.cpp")
target_include_directories(NativeHeightNetworkTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeHeightNetworkTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeHeightNetworkTests PRIVATE Main)
add_test(NAME NativeHeightNetworkTests COMMAND NativeHeightNetworkTests)
add_executable(NativeMapDatabaseTests "${root}/diagnostics/NativeMapDatabaseTests.cpp")
target_include_directories(NativeMapDatabaseTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeMapDatabaseTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeMapDatabaseTests PRIVATE Main)
add_executable(NativeGameDatabaseLoadTests "${root}/diagnostics/NativeGameDatabaseLoadTests.cpp")
target_include_directories(NativeGameDatabaseLoadTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeGameDatabaseLoadTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeGameDatabaseLoadTests PRIVATE Main s2_portable_database)
add_executable(NativeStrProcTests "${root}/diagnostics/NativeStrProcTests.cpp")
target_include_directories(NativeStrProcTests PRIVATE "${root}/Misc" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeStrProcTests PRIVATE "${root}/Misc/StdAfx.h")
target_link_libraries(NativeStrProcTests PRIVATE Misc)
add_test(NAME NativeStrProcTests COMMAND NativeStrProcTests)
add_executable(NativeMapFlagsTests "${root}/diagnostics/NativeMapFlagsTests.cpp")
target_include_directories(NativeMapFlagsTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeMapFlagsTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeMapFlagsTests PRIVATE Main)
add_executable(NativeMissionMapProbe "${root}/diagnostics/NativeMissionMapProbe.cpp")
target_include_directories(NativeMissionMapProbe PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeMissionMapProbe PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeMissionMapProbe PRIVATE Main Script)
target_compile_features(NativeMissionMapProbe PRIVATE cxx_std_17)
add_executable(NativeScriptRouteWaitTests
  "${root}/diagnostics/NativeScriptRouteWaitTests.cpp")
target_include_directories(NativeScriptRouteWaitTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeScriptRouteWaitTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeScriptRouteWaitTests PRIVATE Main Script)
target_compile_features(NativeScriptRouteWaitTests PRIVATE cxx_std_17)
add_test(NAME NativeScriptRouteWaitTests COMMAND NativeScriptRouteWaitTests)
add_executable(NativeAILogicTests "${root}/diagnostics/NativeAILogicTests.cpp")
target_include_directories(NativeAILogicTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeAILogicTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeAILogicTests PRIVATE Main)
target_compile_features(NativeAILogicTests PRIVATE cxx_std_17)
add_test(NAME NativeAILogicTests COMMAND NativeAILogicTests)
add_executable(NativeWorldInitProbe "${root}/diagnostics/NativeWorldInitProbe.cpp")
target_include_directories(NativeWorldInitProbe PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeWorldInitProbe PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeWorldInitProbe PRIVATE Main Script)
target_compile_features(NativeWorldInitProbe PRIVATE cxx_std_17)
add_executable(NativeCommandBridgeTests
  "${root}/diagnostics/NativeCommandBridgeTests.cpp")
target_include_directories(NativeCommandBridgeTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeCommandBridgeTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeCommandBridgeTests PRIVATE Main)
target_compile_features(NativeCommandBridgeTests PRIVATE cxx_std_17)
add_test(NAME NativeCommandBridgeTests COMMAND NativeCommandBridgeTests)
add_executable(NativeAnimationPathTests
  "${root}/diagnostics/NativeAnimationPathTests.cpp")
target_include_directories(NativeAnimationPathTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeAnimationPathTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeAnimationPathTests PRIVATE Main)
target_compile_features(NativeAnimationPathTests PRIVATE cxx_std_17)
add_test(NAME NativeAnimationPathTests COMMAND NativeAnimationPathTests)
add_executable(NativeConsoleConfigTests
  "${root}/diagnostics/NativeConsoleConfigTests.cpp")
target_include_directories(NativeConsoleConfigTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeConsoleConfigTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeConsoleConfigTests PRIVATE Main)
target_compile_features(NativeConsoleConfigTests PRIVATE cxx_std_17)
add_test(NAME NativeConsoleConfigTests COMMAND NativeConsoleConfigTests)
set_tests_properties(NativeConsoleConfigTests PROPERTIES
  ENVIRONMENT "S2_USER_DATA_DIR=${CMAKE_BINARY_DIR}/native-console-user")
add_executable(NativeAttackRulesTests
  "${root}/diagnostics/NativeAttackRulesTests.cpp")
target_include_directories(NativeAttackRulesTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeAttackRulesTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeAttackRulesTests PRIVATE Main)
target_compile_features(NativeAttackRulesTests PRIVATE cxx_std_17)
add_test(NAME NativeAttackRulesTests COMMAND NativeAttackRulesTests)
add_executable(NativeHumanReachTests
  "${root}/diagnostics/NativeHumanReachTests.cpp")
target_include_directories(NativeHumanReachTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeHumanReachTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeHumanReachTests PRIVATE Main)
target_compile_features(NativeHumanReachTests PRIVATE cxx_std_17)
add_test(NAME NativeHumanReachTests COMMAND NativeHumanReachTests)
add_executable(NativePerkPointsTests
  "${root}/diagnostics/NativePerkPointsTests.cpp")
target_include_directories(NativePerkPointsTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativePerkPointsTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativePerkPointsTests PRIVATE Main)
target_compile_features(NativePerkPointsTests PRIVATE cxx_std_17)
add_test(NAME NativePerkPointsTests COMMAND NativePerkPointsTests)
add_executable(NativeDiplomacyBitsTests
  "${root}/diagnostics/NativeDiplomacyBitsTests.cpp")
target_include_directories(NativeDiplomacyBitsTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeDiplomacyBitsTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeDiplomacyBitsTests PRIVATE Main)
target_compile_features(NativeDiplomacyBitsTests PRIVATE cxx_std_17)
add_test(NAME NativeDiplomacyBitsTests COMMAND NativeDiplomacyBitsTests)
add_executable(NativeAITraceSphereTests
  "${root}/diagnostics/NativeAITraceSphereTests.cpp")
target_include_directories(NativeAITraceSphereTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeAITraceSphereTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeAITraceSphereTests PRIVATE Main)
target_compile_features(NativeAITraceSphereTests PRIVATE cxx_std_17)
add_test(NAME NativeAITraceSphereTests COMMAND NativeAITraceSphereTests)
add_executable(NativeExplosionVoxelRendererTests
  "${root}/diagnostics/NativeExplosionVoxelRendererTests.cpp")
target_include_directories(NativeExplosionVoxelRendererTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeExplosionVoxelRendererTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeExplosionVoxelRendererTests PRIVATE Main)
add_test(NAME NativeExplosionVoxelRendererTests COMMAND NativeExplosionVoxelRendererTests)
add_executable(NativeMissionScriptCorpusTests
  "${root}/diagnostics/NativeMissionScriptCorpusTests.cpp")
target_include_directories(NativeMissionScriptCorpusTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeMissionScriptCorpusTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeMissionScriptCorpusTests PRIVATE Main Script)
target_compile_features(NativeMissionScriptCorpusTests PRIVATE cxx_std_17)
add_executable(NativeMapPolygonTests "${root}/diagnostics/NativeMapPolygonTests.cpp")
target_include_directories(NativeMapPolygonTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeMapPolygonTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeMapPolygonTests PRIVATE Main)
target_compile_features(NativeMapPolygonTests PRIVATE cxx_std_17)
add_test(NAME NativeMapPolygonTests COMMAND NativeMapPolygonTests)
add_executable(NativeGeometryCoreTests
  "${root}/diagnostics/NativeGeometryCoreTests.cpp")
target_include_directories(NativeGeometryCoreTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeGeometryCoreTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeGeometryCoreTests PRIVATE Main)
target_compile_features(NativeGeometryCoreTests PRIVATE cxx_std_17)
add_test(NAME NativeGeometryCoreTests COMMAND NativeGeometryCoreTests)
add_executable(NativeSceneClassIDsTests
  "${root}/diagnostics/NativeSceneClassIDsTests.cpp"
  "${root}/Main/GSceneUtils.cpp")
target_include_directories(NativeSceneClassIDsTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeSceneClassIDsTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeSceneClassIDsTests PRIVATE Main)
target_compile_features(NativeSceneClassIDsTests PRIVATE cxx_std_17)
add_test(NAME NativeSceneClassIDsTests COMMAND NativeSceneClassIDsTests)
add_executable(NativeBuildingGridTests "${root}/diagnostics/NativeBuildingGridTests.cpp")
target_include_directories(NativeBuildingGridTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeBuildingGridTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeBuildingGridTests PRIVATE Main)
target_compile_features(NativeBuildingGridTests PRIVATE cxx_std_17)
add_test(NAME NativeBuildingGridTests COMMAND NativeBuildingGridTests)
add_executable(NativeBuildingTerrainResourceTests
  "${root}/diagnostics/NativeBuildingTerrainResourceTests.cpp")
target_include_directories(NativeBuildingTerrainResourceTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeBuildingTerrainResourceTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeBuildingTerrainResourceTests PRIVATE Main)
target_compile_features(NativeBuildingTerrainResourceTests PRIVATE cxx_std_17)
add_executable(NativeResourceOpenerTests "${root}/diagnostics/NativeResourceOpenerTests.cpp")
target_include_directories(NativeResourceOpenerTests PRIVATE "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeResourceOpenerTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeResourceOpenerTests PRIVATE Main)
target_compile_features(NativeResourceOpenerTests PRIVATE cxx_std_17)
add_executable(NativeWaypointResourceTests "${root}/diagnostics/NativeWaypointResourceTests.cpp")
target_include_directories(NativeWaypointResourceTests PRIVATE "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeWaypointResourceTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeWaypointResourceTests PRIVATE Main)
target_compile_features(NativeWaypointResourceTests PRIVATE cxx_std_17)
add_executable(NativeCampaignMapResourceTests
  "${root}/diagnostics/NativeCampaignMapResourceTests.cpp")
target_include_directories(NativeCampaignMapResourceTests PRIVATE "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeCampaignMapResourceTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeCampaignMapResourceTests PRIVATE Main)
target_compile_features(NativeCampaignMapResourceTests PRIVATE cxx_std_17)
add_executable(NativeModManagerTests "${root}/diagnostics/NativeModManagerTests.cpp")
target_include_directories(NativeModManagerTests PRIVATE "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeModManagerTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeModManagerTests PRIVATE Main)
target_compile_features(NativeModManagerTests PRIVATE cxx_std_17)
add_executable(NativeHeadResourceTests "${root}/diagnostics/NativeHeadResourceTests.cpp")
target_include_directories(NativeHeadResourceTests PRIVATE "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeHeadResourceTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeHeadResourceTests PRIVATE Main lifestudio_native_data)
add_executable(NativeHeadSequenceRuntimeTests
  "${root}/diagnostics/NativeHeadSequenceRuntimeTests.cpp")
target_include_directories(NativeHeadSequenceRuntimeTests PRIVATE
  "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeHeadSequenceRuntimeTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeHeadSequenceRuntimeTests PRIVATE Main lifestudio_native_data)
target_compile_features(NativeHeadSequenceRuntimeTests PRIVATE cxx_std_17)
target_compile_features(NativeHeadResourceTests PRIVATE cxx_std_17)
add_executable(NativeAIRouteResourceTests "${root}/diagnostics/NativeAIRouteResourceTests.cpp")
target_include_directories(NativeAIRouteResourceTests PRIVATE "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeAIRouteResourceTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeAIRouteResourceTests PRIVATE Main)
target_compile_features(NativeAIRouteResourceTests PRIVATE cxx_std_17)
add_executable(NativePoolTests "${root}/diagnostics/NativePoolTests.cpp")
target_include_directories(NativePoolTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativePoolTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativePoolTests COMMAND NativePoolTests)
add_executable(NativeSelectionInfoWireTests "${root}/diagnostics/NativeSelectionInfoWireTests.cpp")
target_include_directories(NativeSelectionInfoWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeSelectionInfoWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeSelectionInfoWireTests COMMAND NativeSelectionInfoWireTests)
add_executable(NativeRenderStateWireTests "${root}/diagnostics/NativeRenderStateWireTests.cpp")
target_include_directories(NativeRenderStateWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeRenderStateWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeRenderStateWireTests COMMAND NativeRenderStateWireTests)
add_executable(NativeLightKeyWireTests "${root}/diagnostics/NativeLightKeyWireTests.cpp")
target_include_directories(NativeLightKeyWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeLightKeyWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeLightKeyWireTests COMMAND NativeLightKeyWireTests)
add_executable(NativeBoolSyncRuleTests "${root}/diagnostics/NativeBoolSyncRuleTests.cpp")
target_include_directories(NativeBoolSyncRuleTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeBoolSyncRuleTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeBoolSyncRuleTests COMMAND NativeBoolSyncRuleTests)
add_executable(NativeFontWireTests "${root}/diagnostics/NativeFontWireTests.cpp")
target_include_directories(NativeFontWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeFontWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeFontWireTests COMMAND NativeFontWireTests)
add_executable(NativeAITileWireTests "${root}/diagnostics/NativeAITileWireTests.cpp")
target_include_directories(NativeAITileWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeAITileWireTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeAITileWireTests PRIVATE Main)
add_test(NAME NativeAITileWireTests COMMAND NativeAITileWireTests)
add_executable(NativeVisionQueryWireTests "${root}/diagnostics/NativeVisionQueryWireTests.cpp")
target_include_directories(NativeVisionQueryWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeVisionQueryWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeVisionQueryWireTests COMMAND NativeVisionQueryWireTests)
add_executable(NativeMeshWireTests "${root}/diagnostics/NativeMeshWireTests.cpp")
target_include_directories(NativeMeshWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeMeshWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeMeshWireTests COMMAND NativeMeshWireTests)
add_executable(NativeClipShareWireTests "${root}/diagnostics/NativeClipShareWireTests.cpp")
target_include_directories(NativeClipShareWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeClipShareWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeClipShareWireTests COMMAND NativeClipShareWireTests)
add_executable(NativePartFlagsWireTests "${root}/diagnostics/NativePartFlagsWireTests.cpp")
target_include_directories(NativePartFlagsWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativePartFlagsWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativePartFlagsWireTests COMMAND NativePartFlagsWireTests)
add_executable(NativeBuildingPartTests "${root}/diagnostics/NativeBuildingPartTests.cpp")
target_include_directories(NativeBuildingPartTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeBuildingPartTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeBuildingPartTests COMMAND NativeBuildingPartTests)
add_executable(NativeSquareMoveWireTests "${root}/diagnostics/NativeSquareMoveWireTests.cpp")
target_include_directories(NativeSquareMoveWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeSquareMoveWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeSquareMoveWireTests COMMAND NativeSquareMoveWireTests)
add_executable(NativeMapStoreyWireTests "${root}/diagnostics/NativeMapStoreyWireTests.cpp")
target_include_directories(NativeMapStoreyWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeMapStoreyWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeMapStoreyWireTests COMMAND NativeMapStoreyWireTests)
add_executable(NativeAnimationWireTests "${root}/diagnostics/NativeAnimationWireTests.cpp")
target_include_directories(NativeAnimationWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeAnimationWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeAnimationWireTests COMMAND NativeAnimationWireTests)
add_executable(NativeCameraEffectWireTests "${root}/diagnostics/NativeCameraEffectWireTests.cpp")
target_include_directories(NativeCameraEffectWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeCameraEffectWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeCameraEffectWireTests COMMAND NativeCameraEffectWireTests)
add_executable(NativeLuaStateWireTests "${root}/diagnostics/NativeLuaStateWireTests.cpp")
target_include_directories(NativeLuaStateWireTests PRIVATE "${root}/Script" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeLuaStateWireTests PRIVATE "${root}/Script/StdAfx.h")
add_test(NAME NativeLuaStateWireTests COMMAND NativeLuaStateWireTests)
add_executable(NativeAIDistanceWireTests "${root}/diagnostics/NativeAIDistanceWireTests.cpp")
target_include_directories(NativeAIDistanceWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeAIDistanceWireTests PRIVATE "${root}/Main/StdAfx.h")
target_link_libraries(NativeAIDistanceWireTests PRIVATE Main)
add_test(NAME NativeAIDistanceWireTests COMMAND NativeAIDistanceWireTests)
add_executable(NativeParticleWireTests "${root}/diagnostics/NativeParticleWireTests.cpp")
target_include_directories(NativeParticleWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeParticleWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeParticleWireTests COMMAND NativeParticleWireTests)
add_executable(NativeExplosionPerkWireTests "${root}/diagnostics/NativeExplosionPerkWireTests.cpp")
target_include_directories(NativeExplosionPerkWireTests PRIVATE "${root}/Main" "${CMAKE_BINARY_DIR}/rc_compat")
target_precompile_headers(NativeExplosionPerkWireTests PRIVATE "${root}/Main/StdAfx.h")
add_test(NAME NativeExplosionPerkWireTests COMMAND NativeExplosionPerkWireTests)
# the game
s2_module(Game win32 WHOLE Main
                     LINK ADOImport DBFormat FileIO FModSound Input MiscDll Misc Script
                          fmod_shim lifestudio_shim bink_shim
                          dxguid dinput8 odbc32 odbccp32 d3d9 user32 Gdi32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  # The default x64 configuration still uses link-only media stubs. Native
  # FFmpeg/miniaudio targets below enable game audio/video when requested.
  target_compile_definitions(Game PRIVATE S2_X64_MEDIA_STUBS)
  target_compile_definitions(Main PRIVATE S2_X64_MEDIA_STUBS)
endif()
option(S2_ENABLE_NATIVE_MUSIC "Use FFmpeg/miniaudio for x64 game music" OFF)
if(S2_ENABLE_NATIVE_MUSIC)
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR NOT S2_BUILD_MINIAUDIO_MUSIC_PROBE)
    message(FATAL_ERROR "S2_ENABLE_NATIVE_MUSIC requires x64 and the FFmpeg/miniaudio targets")
  endif()
  target_link_libraries(FModSound PUBLIC s2_native_music)
  foreach(target IN ITEMS FModSound Main Game)
    target_compile_definitions(${target} PRIVATE S2_NATIVE_MUSIC)
  endforeach()
endif()
if(S2_ENABLE_NATIVE_SFX)
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR NOT S2_ENABLE_NATIVE_MUSIC)
    message(FATAL_ERROR "S2_ENABLE_NATIVE_SFX requires x64 native music/media")
  endif()
  add_library(s2_native_fmod STATIC "${root}/Media/NativeFmodCompat.cpp")
  target_include_directories(s2_native_fmod PRIVATE
    "${root}/third_party/fmod/include" "${S2_MINIAUDIO_INCLUDE_DIR}")
  target_compile_features(s2_native_fmod PRIVATE cxx_std_17)
  target_link_libraries(s2_native_fmod PUBLIC s2_native_music)
  target_link_libraries(fmod_shim INTERFACE s2_native_fmod)
  target_compile_definitions(FModSound PRIVATE S2_NATIVE_SFX)
  add_executable(NativeFmodProbe "${root}/diagnostics/NativeFmodProbe.cpp")
  target_link_libraries(NativeFmodProbe PRIVATE fmod_shim)
endif()
option(S2_ENABLE_NATIVE_VIDEO "Use FFmpeg/miniaudio for x64 Bink movies" OFF)
if(S2_ENABLE_NATIVE_VIDEO)
  if(NOT S2_ENABLE_NATIVE_MUSIC)
    message(FATAL_ERROR "S2_ENABLE_NATIVE_VIDEO requires S2_ENABLE_NATIVE_MUSIC")
  endif()
  add_library(s2_bink_compat SHARED "${root}/Media/BinkCompat.cpp")
  target_include_directories(s2_bink_compat PRIVATE
    "${root}/third_party/bink/include")
  target_compile_features(s2_bink_compat PRIVATE cxx_std_17)
  target_compile_definitions(s2_bink_compat PRIVATE S2_BINK_COMPAT_EXPORTS)
  target_link_libraries(s2_bink_compat PRIVATE s2_native_music)
  set_target_properties(s2_bink_compat PROPERTIES OUTPUT_NAME binkw32
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/_media"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/_media"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/_media")
  target_compile_definitions(Main PRIVATE S2_NATIVE_VIDEO)
  add_executable(BinkCompatProbe "${root}/diagnostics/BinkCompatProbe.cpp")
  target_include_directories(BinkCompatProbe PRIVATE
    "${root}/third_party/bink/include")
  target_link_libraries(BinkCompatProbe PRIVATE s2_bink_compat)
endif()
if(TARGET s2_bink_compat)
  set_tests_properties(NativeZoneWireTests NativeObjectPlaceTests NativeCriticalTests NativeJunctionTests NativeAITileWireTests NativeAIDistanceWireTests NativeHeightLayersTests NativeHeightNetworkTests PROPERTIES ENVIRONMENT_MODIFICATION
    "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
endif()
add_dependencies(Game ls_implibs)   # the lib/def import libs must exist before Game links
# tools
s2_module(DataImport     console LINK DBFormat FileIO Misc odbc32 odbccp32 DEFINES _CONSOLE)
s2_module(PkgBuilder     console LINK FileIO Misc odbc32 odbccp32 DEFINES _CONSOLE)
# ShaderCompiler needs legacy D3DX9 (dropped from the modern Windows SDK). Search
# $(DXSDK_DIR) and the standard DirectX SDK installs (June 2010 / August 2007).
# Built only if d3dx9.lib is found; override with -DD3DX9_LIB=<path>.
set(_dx_roots
  "$ENV{DXSDK_DIR}"
  "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)"
  "C:/Program Files (x86)/Microsoft DirectX SDK (August 2007)")
set(_dx_libdirs "")
foreach(_r IN LISTS _dx_roots)
  if(IS_DIRECTORY "${_r}")
    list(APPEND _dx_libdirs "${_r}/Lib/${S2_MSVC_MACHINE}")
  endif()
endforeach()
find_library(D3DX9_LIB d3dx9 PATHS ${_dx_libdirs})
if(D3DX9_LIB)
  # The legacy DirectX SDK's Include dir also ships OLD sal.h / rpcsal.h / specstrings.h
  # that shadow the modern Windows SDK's and break its COM headers (objidl.h etc. use
  # __RPC__*_xcount, undefined by the old rpcsal.h). So expose ONLY the DirectX-unique
  # headers to the compiler -- shared headers then resolve against the Windows SDK.
  get_filename_component(_dxroot "${D3DX9_LIB}/../../.." ABSOLUTE)   # .../Lib/x86/d3dx9.lib -> SDK root
  set(_dxinc "${CMAKE_BINARY_DIR}/dxsdk_d3dx9_include")
  file(GLOB _dxhdrs "${_dxroot}/Include/d3dx9*.h" "${_dxroot}/Include/rmxf*.h"
                    "${_dxroot}/Include/dxfile.h" "${_dxroot}/Include/dxerr*.h"
                    "${_dxroot}/Include/d3dx9*.inl")
  file(COPY ${_dxhdrs} DESTINATION "${_dxinc}")
  s2_module(ShaderCompiler console LINK FileIO Misc odbc32 odbccp32 "${D3DX9_LIB}" DEFINES _CONSOLE)
  target_include_directories(ShaderCompiler PRIVATE "${_dxinc}")
  message(STATUS "ShaderCompiler: D3DX9 = ${D3DX9_LIB}")
else()
  message(STATUS "ShaderCompiler skipped: D3DX9 not found (install the DirectX SDK or set DXSDK_DIR env var).")
endif()
s2_module(FontGen        console LINK FileIO Image libpng Misc zlib squish odbc32 odbccp32 DEFINES _CONSOLE)
s2_module(TexConv        console LINK FileIO Image libpng Misc zlib squish DEFINES _CONSOLE)
s2_module(TexMipStrip    console LINK FileIO Image Misc squish DEFINES _CONSOLE)
s2_module(LSConverter  console LINK FileIO Image libpng Misc squish lifestudio_shim DEFINES _CONSOLE)

# Kept on disk but intentionally NOT built (sources preserved for reference):
#   MapEdit      -- needs the proprietary Stingray + LifeMode SDKs; does not compile.
#   Scintilla    -- only used by MapEdit.
#   OpenDynamix  -- only used by MapEdit (ODE physics; would also need ODE headers).

# ----------------------------------------------------------------------------
#  Visual Studio debugging
# ----------------------------------------------------------------------------
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Game)

set(S2_GAME_DIR "" CACHE PATH "Silent Storm install dir")
if(NOT S2_GAME_DIR)
  foreach(_d "C:/Program Files (x86)/Steam/steamapps/common/Silent Storm"
             "C:/Program Files/Steam/steamapps/common/Silent Storm")
    if(EXISTS "${_d}")
      set(S2_GAME_DIR "${_d}" CACHE PATH "" FORCE)
      break()
    endif()
  endforeach()
endif()
if(S2_GAME_DIR)
  set_target_properties(Game PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${S2_GAME_DIR}"
    VS_DEBUGGER_ENVIRONMENT "PATH=${S2_GAME_DIR};%PATH%")
  if(TARGET s2_bink_compat)
    set_tests_properties(NativeScriptRouteWaitTests PROPERTIES ENVIRONMENT_MODIFICATION
      "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
    set_tests_properties(NativeAILogicTests PROPERTIES ENVIRONMENT_MODIFICATION
      "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
    set_tests_properties(NativeCommandBridgeTests PROPERTIES ENVIRONMENT_MODIFICATION
      "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
  else()
    set_tests_properties(NativeScriptRouteWaitTests PROPERTIES ENVIRONMENT_MODIFICATION
      "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
    set_tests_properties(NativeAILogicTests PROPERTIES ENVIRONMENT_MODIFICATION
      "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
    set_tests_properties(NativeCommandBridgeTests PROPERTIES ENVIRONMENT_MODIFICATION
      "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
  endif()
  message(STATUS "Game working dir: ${S2_GAME_DIR}")
  if(EXISTS "${S2_GAME_DIR}/game.db")
    if(EXISTS "${S2_GAME_DIR}/scripts/Hint.l" AND
       EXISTS "${S2_GAME_DIR}/scripts/Common.l" AND
       EXISTS "${S2_GAME_DIR}/scripts/TriggersManager.l" AND
       EXISTS "${S2_GAME_DIR}/scripts/Constants.l" AND
       EXISTS "${S2_GAME_DIR}/res/Waypoints.res")
      add_test(NAME NativeWorldInitProbe
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res")
      set_tests_properties(NativeWorldInitProbe PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 120)
      # Exercise authored routes and two scenario-root campaign maps in the
      # full world path as well as the small bootstrap variants.
      foreach(_s2_world_variant IN ITEMS 218 810 2223 3829 4526 5240)
        add_test(NAME NativeWorldMission${_s2_world_variant}
          COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
            --mission ${_s2_world_variant})
        set_tests_properties(NativeWorldMission${_s2_world_variant} PROPERTIES
          WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 300)
      endforeach()
      add_test(NAME NativeScenarioRootWorlds
        COMMAND "${CMAKE_COMMAND}"
          "-DDB_TEST=$<TARGET_FILE:NativeMapDatabaseTests>"
          "-DWORLD_PROBE=$<TARGET_FILE:NativeWorldInitProbe>"
          "-DGAME_DB=${S2_GAME_DIR}/game.db"
          "-DRESOURCE_DIR=${S2_GAME_DIR}/res"
          -P "${root}/diagnostics/RunScenarioRootWorlds.cmake")
      set_tests_properties(NativeScenarioRootWorlds PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 7200 LABELS extended)
      add_test(NAME NativeScenarioRootPartyWorlds
        COMMAND "${CMAKE_COMMAND}"
          "-DDB_TEST=$<TARGET_FILE:NativeMapDatabaseTests>"
          "-DWORLD_PROBE=$<TARGET_FILE:NativeWorldInitProbe>"
          "-DGAME_DB=${S2_GAME_DIR}/game.db"
          "-DRESOURCE_DIR=${S2_GAME_DIR}/res"
          -DPARTY_MODE=ON
          -P "${root}/diagnostics/RunScenarioRootWorlds.cmake")
      set_tests_properties(NativeScenarioRootPartyWorlds PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 7200 LABELS extended)
      add_test(NAME NativeScenarioRootPartySaveWorlds
        COMMAND "${CMAKE_COMMAND}"
          "-DDB_TEST=$<TARGET_FILE:NativeMapDatabaseTests>"
          "-DWORLD_PROBE=$<TARGET_FILE:NativeWorldInitProbe>"
          "-DGAME_DB=${S2_GAME_DIR}/game.db"
          "-DRESOURCE_DIR=${S2_GAME_DIR}/res"
          "-DSAVE_DIR=${CMAKE_BINARY_DIR}/_root_party_saves/all"
          -DPARTY_SAVE_MODE=ON
          -P "${root}/diagnostics/RunScenarioRootWorlds.cmake")
      set_tests_properties(NativeScenarioRootPartySaveWorlds PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 10800 LABELS extended)
      add_test(NAME NativeWorldMission5247PartySave
        COMMAND "${CMAKE_COMMAND}"
          "-DDB_TEST=$<TARGET_FILE:NativeMapDatabaseTests>"
          "-DWORLD_PROBE=$<TARGET_FILE:NativeWorldInitProbe>"
          "-DGAME_DB=${S2_GAME_DIR}/game.db"
          "-DRESOURCE_DIR=${S2_GAME_DIR}/res"
          "-DSAVE_DIR=${CMAKE_BINARY_DIR}/_root_party_saves/5247"
          -DPARTY_SAVE_MODE=ON
          -DSTART_VARIANT=5247
          -DEND_VARIANT=5247
          -P "${root}/diagnostics/RunScenarioRootWorlds.cmake")
      set_tests_properties(NativeWorldMission5247PartySave PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 300)
      add_test(NAME NativeWorldMission5247PartySaveTurn
        COMMAND "${CMAKE_COMMAND}"
          "-DDB_TEST=$<TARGET_FILE:NativeMapDatabaseTests>"
          "-DWORLD_PROBE=$<TARGET_FILE:NativeWorldInitProbe>"
          "-DGAME_DB=${S2_GAME_DIR}/game.db"
          "-DRESOURCE_DIR=${S2_GAME_DIR}/res"
          "-DSAVE_DIR=${CMAKE_BINARY_DIR}/_root_party_saves/5247-turn"
          -DPARTY_SAVE_MODE=ON
          -DTURN_MODE=ON
          -DSTART_VARIANT=5247
          -DEND_VARIANT=5247
          -P "${root}/diagnostics/RunScenarioRootWorlds.cmake")
      set_tests_properties(NativeWorldMission5247PartySaveTurn PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 300)
      add_test(NAME NativeWorldMission3832PartyUIAck
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-root-party 3832)
      set_tests_properties(NativeWorldMission3832PartyUIAck PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 300)
      add_test(NAME NativeWorldMission6814PartyUIAck
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-root-party 6814)
      set_tests_properties(NativeWorldMission6814PartyUIAck PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 300)
      add_test(NAME NativeWorldMission810UIAck
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-ui-ack 810)
      set_tests_properties(NativeWorldMission810UIAck PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 120)
      add_test(NAME NativeWorldBase5376UIAck
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-base-party-ui-ack 5376)
      set_tests_properties(NativeWorldBase5376UIAck PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 120)
      add_test(NAME NativeWorldMission810PartyUIAck
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-party-ui-ack 810)
      set_tests_properties(NativeWorldMission810PartyUIAck PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 120)
      add_test(NAME NativeWorldMission810PartyShot
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-party-shot 810)
      set_tests_properties(NativeWorldMission810PartyShot PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 120)
      add_test(NAME NativeWorldMission810PartyShotSave
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-party-shot-save 810 "${CMAKE_BINARY_DIR}/world-mission-810-shot.sav")
      set_tests_properties(NativeWorldMission810PartyShotSave PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 120)
      add_test(NAME NativeWorldMission810PartyShotSlot
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-party-shot-slot 810)
      set_tests_properties(NativeWorldMission810PartyShotSlot PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 120
        ENVIRONMENT "S2_USER_DATA_DIR=${CMAKE_BINARY_DIR}/world-slot-user")
      add_test(NAME NativeWorldMission810PartyExplosionSave
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-party-explosion-save 810 "${CMAKE_BINARY_DIR}/world-mission-810-explosion.sav")
      set_tests_properties(NativeWorldMission810PartyExplosionSave PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 120)
      add_test(NAME NativeWorldMission810PartyGrenadeSave
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-party-grenade-save 810 "${CMAKE_BINARY_DIR}/world-mission-810-grenade.sav")
      set_tests_properties(NativeWorldMission810PartyGrenadeSave PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 120)
      add_test(NAME NativeWorldMission810PartyGrenadeFlightSave
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-party-grenade-flight-save 810 "${CMAKE_BINARY_DIR}/world-mission-810-grenade-flight.sav")
      set_tests_properties(NativeWorldMission810PartyGrenadeFlightSave PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 120)
      add_test(NAME NativeWorldMission810PartyGrenadeInventorySave
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-party-grenade-inventory-save 810 "${CMAKE_BINARY_DIR}/world-mission-810-grenade-inventory.sav")
      set_tests_properties(NativeWorldMission810PartyGrenadeInventorySave PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 120)
      add_test(NAME NativeWorldMission810PartyEngGrenadeInventorySave
        COMMAND NativeWorldInitProbe "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res"
          --mission-party-eng-grenade-inventory-save 810 "${CMAKE_BINARY_DIR}/world-mission-810-eng-grenade-inventory.sav")
      set_tests_properties(NativeWorldMission810PartyEngGrenadeInventorySave PROPERTIES
        WORKING_DIRECTORY "${S2_GAME_DIR}" TIMEOUT 120)
      if(TARGET s2_bink_compat)
        set_tests_properties(NativeWorldInitProbe NativeWorldMission218 NativeWorldMission810 NativeWorldMission2223 NativeWorldMission3829 NativeWorldMission4526 NativeWorldMission5240 NativeScenarioRootWorlds NativeScenarioRootPartyWorlds NativeWorldMission3832PartyUIAck NativeWorldMission6814PartyUIAck NativeWorldMission810UIAck NativeWorldBase5376UIAck NativeWorldMission810PartyUIAck NativeWorldMission810PartyShot NativeWorldMission810PartyShotSave NativeWorldMission810PartyShotSlot NativeWorldMission810PartyExplosionSave NativeWorldMission810PartyGrenadeSave NativeWorldMission810PartyGrenadeFlightSave NativeWorldMission810PartyGrenadeInventorySave NativeWorldMission810PartyEngGrenadeInventorySave
          PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
      else()
        set_tests_properties(NativeWorldInitProbe NativeWorldMission218 NativeWorldMission810 NativeWorldMission2223 NativeWorldMission3829 NativeWorldMission4526 NativeWorldMission5240 NativeScenarioRootWorlds NativeScenarioRootPartyWorlds NativeWorldMission3832PartyUIAck NativeWorldMission6814PartyUIAck NativeWorldMission810UIAck NativeWorldBase5376UIAck NativeWorldMission810PartyUIAck NativeWorldMission810PartyShot NativeWorldMission810PartyShotSave NativeWorldMission810PartyShotSlot NativeWorldMission810PartyExplosionSave NativeWorldMission810PartyGrenadeSave NativeWorldMission810PartyGrenadeFlightSave NativeWorldMission810PartyGrenadeInventorySave NativeWorldMission810PartyEngGrenadeInventorySave
          PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
      endif()
      if(TARGET s2_bink_compat)
        set_tests_properties(NativeScenarioRootPartySaveWorlds NativeWorldMission5247PartySave NativeWorldMission5247PartySaveTurn PROPERTIES
          ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
      else()
        set_tests_properties(NativeScenarioRootPartySaveWorlds NativeWorldMission5247PartySave NativeWorldMission5247PartySaveTurn PROPERTIES
          ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
      endif()
    endif()
    add_test(NAME NativeMapDatabaseTests
      COMMAND NativeMapDatabaseTests "${S2_GAME_DIR}/game.db")
    add_test(NAME NativeGameDatabaseLoadTests
      COMMAND NativeGameDatabaseLoadTests "${S2_GAME_DIR}/game.db")
    add_test(NAME NativeMissionScriptCorpusTests
      COMMAND NativeMissionScriptCorpusTests "${S2_GAME_DIR}/game.db")
    add_test(NAME NativeMapFlagsTests
      COMMAND NativeMapFlagsTests "${S2_GAME_DIR}/game.db")
    if(EXISTS "${S2_GAME_DIR}/res/Buildings.res" AND
       EXISTS "${S2_GAME_DIR}/res/Terrain.res" AND
       EXISTS "${S2_GAME_DIR}/res/AIGeometries.res")
      add_test(NAME NativeBuildingTerrainResourceTests
        COMMAND NativeBuildingTerrainResourceTests "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res")
      if(EXISTS "${S2_GAME_DIR}/res/Waypoints.res")
        add_test(NAME NativeMissionMapProbe
          COMMAND NativeMissionMapProbe "${S2_GAME_DIR}/game.db"
            "${S2_GAME_DIR}/res" 218)
        if(EXISTS "${S2_GAME_DIR}/res/Units.res" AND
           EXISTS "${S2_GAME_DIR}/res/Groups.res")
          add_test(NAME NativeMissionRouteMapProbe
            COMMAND NativeMissionMapProbe "${S2_GAME_DIR}/game.db"
              "${S2_GAME_DIR}/res" 810)
          add_test(NAME NativeMissionUnitRouteMapProbe
            COMMAND NativeMissionMapProbe "${S2_GAME_DIR}/game.db"
              "${S2_GAME_DIR}/res" 2400)
          add_test(NAME NativeMissionLargeMapProbe
            COMMAND NativeMissionMapProbe "${S2_GAME_DIR}/game.db"
              "${S2_GAME_DIR}/res" 4526)
          add_test(NAME NativeScenarioPlacement3830
            COMMAND NativeMissionMapProbe "${S2_GAME_DIR}/game.db"
              "${S2_GAME_DIR}/res" 3830 --deterministic)
          add_test(NAME NativeScenarioRootMaps
            COMMAND "${CMAKE_COMMAND}"
              "-DDB_TEST=$<TARGET_FILE:NativeMapDatabaseTests>"
              "-DMAP_PROBE=$<TARGET_FILE:NativeMissionMapProbe>"
              "-DGAME_DB=${S2_GAME_DIR}/game.db"
              "-DRESOURCE_DIR=${S2_GAME_DIR}/res"
              -P "${root}/diagnostics/RunScenarioRootMaps.cmake")
          set_tests_properties(NativeScenarioRootMaps PROPERTIES TIMEOUT 7200 LABELS extended)
        endif()
      endif()
    endif()
    if(TARGET s2_bink_compat)
      set_tests_properties(NativeMissionScriptCorpusTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
      set_tests_properties(NativeMapFlagsTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
      if(TEST NativeBuildingTerrainResourceTests)
        set_tests_properties(NativeBuildingTerrainResourceTests PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
      endif()
      if(TEST NativeMissionMapProbe)
        set_tests_properties(NativeMissionMapProbe PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
      endif()
      if(TEST NativeMissionRouteMapProbe)
        set_tests_properties(NativeMissionRouteMapProbe PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
      endif()
      if(TEST NativeMissionUnitRouteMapProbe)
        set_tests_properties(NativeMissionUnitRouteMapProbe PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
      endif()
      if(TEST NativeMissionLargeMapProbe)
        set_tests_properties(NativeMissionLargeMapProbe PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
      endif()
      if(TEST NativeScenarioPlacement3830)
        set_tests_properties(NativeScenarioPlacement3830 PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
      endif()
      if(TEST NativeScenarioRootMaps)
        set_tests_properties(NativeScenarioRootMaps PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
      endif()
    else()
      set_tests_properties(NativeMissionScriptCorpusTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
      set_tests_properties(NativeMapFlagsTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
      if(TEST NativeBuildingTerrainResourceTests)
        set_tests_properties(NativeBuildingTerrainResourceTests PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
      endif()
      if(TEST NativeMissionMapProbe)
        set_tests_properties(NativeMissionMapProbe PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
      endif()
      if(TEST NativeScenarioRootMaps)
        set_tests_properties(NativeScenarioRootMaps PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
      endif()
      if(TEST NativeScenarioPlacement3830)
        set_tests_properties(NativeScenarioPlacement3830 PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
      endif()
      if(TEST NativeMissionRouteMapProbe)
        set_tests_properties(NativeMissionRouteMapProbe PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
      endif()
      if(TEST NativeMissionUnitRouteMapProbe)
        set_tests_properties(NativeMissionUnitRouteMapProbe PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
      endif()
      if(TEST NativeMissionLargeMapProbe)
        set_tests_properties(NativeMissionLargeMapProbe PROPERTIES ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
      endif()
    endif()
  endif()
  if(EXISTS "${S2_GAME_DIR}/res/Waypoints.res")
    add_test(NAME NativeResourcePackageTests
      COMMAND NativeResourcePackageTests "${S2_GAME_DIR}/res/Waypoints.res")
    if(EXISTS "${S2_GAME_DIR}/res/Textures.res" AND
       EXISTS "${S2_GAME_DIR}/res/Sounds.res")
      add_test(NAME NativeResourceCorpusTests
        COMMAND NativeResourcePackageTests --corpus "${S2_GAME_DIR}/res")
      set_tests_properties(NativeResourceCorpusTests PROPERTIES TIMEOUT 600)
    endif()
    add_test(NAME NativeResourceOpenerTests
      COMMAND NativeResourceOpenerTests "${S2_GAME_DIR}/res/Waypoints.res"
        "${CMAKE_BINARY_DIR}/resource-opener-fixture")
    add_test(NAME NativeWaypointResourceTests
      COMMAND NativeWaypointResourceTests "${S2_GAME_DIR}/res/Waypoints.res")
    if(EXISTS "${S2_GAME_DIR}/res/Chapters.res" AND
       EXISTS "${S2_GAME_DIR}/res/Globals.res")
      add_test(NAME NativeCampaignMapResourceTests
        COMMAND NativeCampaignMapResourceTests
          "${S2_GAME_DIR}/res/Chapters.res" "${S2_GAME_DIR}/res/Globals.res")
    endif()
    if(EXISTS "${S2_GAME_DIR}/game.db")
      add_test(NAME NativeModManagerTests COMMAND NativeModManagerTests
        "${S2_GAME_DIR}/game.db" "${CMAKE_BINARY_DIR}/mod-manager-scratch")
    endif()
    if(TARGET s2_bink_compat)
      set_tests_properties(NativeResourceOpenerTests NativeWaypointResourceTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
    else()
      set_tests_properties(NativeResourceOpenerTests NativeWaypointResourceTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
    endif()
  endif()
  if(EXISTS "${S2_GAME_DIR}/res/Heads.res")
    add_test(NAME NativeHeadResourceTests
      COMMAND NativeHeadResourceTests "${S2_GAME_DIR}/res/Heads.res" "${S2_GAME_DIR}/tree.mma" "${S2_GAME_DIR}/game.db")
    if(TARGET s2_bink_compat)
      set_tests_properties(NativeHeadResourceTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
    else()
      set_tests_properties(NativeHeadResourceTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
    endif()
  endif()
  if(EXISTS "${S2_GAME_DIR}/res/Sequences.res" AND EXISTS "${S2_GAME_DIR}/tree.mma")
    add_test(NAME NativeHeadSequenceRuntimeTests
      COMMAND NativeHeadSequenceRuntimeTests "${S2_GAME_DIR}/res/Sequences.res" "${S2_GAME_DIR}/tree.mma")
    if(TARGET s2_bink_compat)
      set_tests_properties(NativeHeadSequenceRuntimeTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
    else()
      set_tests_properties(NativeHeadSequenceRuntimeTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
    endif()
  endif()
  if(EXISTS "${S2_GAME_DIR}/game.db" AND
     EXISTS "${S2_GAME_DIR}/res/Units.res" AND
     EXISTS "${S2_GAME_DIR}/res/Groups.res")
    add_test(NAME NativeAIRouteResourceTests
      COMMAND NativeAIRouteResourceTests "${S2_GAME_DIR}/game.db" "${S2_GAME_DIR}/res")
    if(TARGET s2_bink_compat)
      set_tests_properties(NativeAIRouteResourceTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:s2_bink_compat>;PATH=path_list_prepend:${S2_FFMPEG_ROOT}/bin")
    else()
      set_tests_properties(NativeAIRouteResourceTests PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
    endif()
  endif()
else()
  message(STATUS "Game install not found -- pass -DS2_GAME_DIR=\"<path>\" to set the working dir.")
endif()
if(S2_GAME_DIR AND NOT TARGET s2_bink_compat)
  set_tests_properties(NativeAITileWireTests NativeZoneWireTests NativeAIDistanceWireTests NativeHeightLayersTests NativeHeightNetworkTests PROPERTIES ENVIRONMENT_MODIFICATION
    "PATH=path_list_prepend:${S2_GAME_DIR};PATH=path_list_prepend:${CMAKE_BINARY_DIR}/_implibs")
endif()
