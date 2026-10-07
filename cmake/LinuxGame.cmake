find_package(SDL3 3.4.16 EXACT CONFIG REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "The full Linux game requires Clang for historical anonymous union members")
endif()
set(_s2_game_root "${CMAKE_BINARY_DIR}/linux-sources")
# Regenerate the build-only include-normalized copy after source edits, too.
# Watching only file names leaves incremental builds using an older copy.
set(_s2_prepare_inputs "${root}/diagnostics/PrepareLinuxSources.py")
foreach(module IN ITEMS Misc MiscDll FileIO Image ADOImport DBFormat Input Script FModSound Main Game Media third_party diagnostics)
  file(GLOB_RECURSE _s2_module_inputs CONFIGURE_DEPENDS
    "${root}/${module}/*.h" "${root}/${module}/*.cpp" "${root}/${module}/*.c" "${root}/${module}/*.inl")
  list(APPEND _s2_prepare_inputs ${_s2_module_inputs})
endforeach()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_s2_prepare_inputs})
execute_process(COMMAND "${Python3_EXECUTABLE}" "${root}/diagnostics/PrepareLinuxSources.py"
  --source "${root}" --output "${_s2_game_root}" COMMAND_ERROR_IS_FATAL ANY)
include("${root}/sources.cmake")
include("${root}/cmake/Bgfx.cmake")

add_library(s2_platform STATIC "${root}/Game/Platform.cpp")
target_link_libraries(s2_platform PUBLIC SDL3::SDL3)
target_compile_features(s2_platform PUBLIC cxx_std_17)
if(NOT TARGET s2_native_music)
  message(FATAL_ERROR "The Linux game requires S2_BUILD_MEDIA_PROBE and S2_BUILD_MINIAUDIO_MUSIC_PROBE")
endif()
target_link_libraries(s2_native_music PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
add_library(s2_native_fmod STATIC "${root}/Media/NativeFmodCompat.cpp")
target_include_directories(s2_native_fmod PRIVATE "${root}/third_party/fmod/include" "${S2_MINIAUDIO_INCLUDE_DIR}")
target_compile_options(s2_native_fmod PRIVATE -include "${_s2_game_root}/Misc/GamePlatform.h")
target_link_libraries(s2_native_fmod PUBLIC s2_native_music)
add_library(s2_bink_compat STATIC "${root}/Media/BinkCompat.cpp")
target_include_directories(s2_bink_compat PUBLIC "${root}/third_party/bink/include")
target_compile_options(s2_bink_compat PRIVATE -include "${_s2_game_root}/Misc/GamePlatform.h")
target_link_libraries(s2_bink_compat PUBLIC s2_native_music)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSF REQUIRED IMPORTED_TARGET libgsf-1)
target_sources(s2_native_lifestudio PRIVATE
  "${root}/third_party/lifestudio/src/NativeGDP.cpp"
  "${root}/third_party/lifestudio/src/lsglue.cpp")
target_link_libraries(s2_native_lifestudio PUBLIC PkgConfig::GSF)

list(REMOVE_ITEM Main_SRC Gfx.cpp)
# The SDL application no longer calls the historical GDI startup splash.
list(REMOVE_ITEM Main_SRC SplashScreen.cpp SplashScreenDialog.cpp)
list(REMOVE_ITEM Main_SRC iSaveManager.cpp)
list(APPEND Main_SRC GfxBgfx.cpp BgfxBackend.cpp iSaveManagerLinux.cpp)
list(REMOVE_ITEM FileIO_SRC WindowsUserData.cpp WindowsSaveNames.cpp)
list(APPEND FileIO_SRC LinuxUserData.cpp LinuxSaveNames.cpp)
foreach(module IN ITEMS Misc FileIO MiscDll Image ADOImport DBFormat Input Script FModSound Main Game)
  set(source_files "")
  file(GLOB module_files "${_s2_game_root}/${module}/*")
  foreach(relative IN LISTS ${module}_SRC)
    if(NOT relative MATCHES "\\.rc$|^StdAfx\\.cpp$")
      set(source_found FALSE)
      string(TOLOWER "${relative}" relative_lower)
      foreach(candidate IN LISTS module_files)
        get_filename_component(candidate_name "${candidate}" NAME)
        string(TOLOWER "${candidate_name}" candidate_lower)
        if(candidate_lower STREQUAL relative_lower)
          list(APPEND source_files "${candidate}")
          set(source_found TRUE)
          break()
        endif()
      endforeach()
      if(NOT source_found)
        message(FATAL_ERROR "Missing source in ${module}: ${relative}")
      endif()
    endif()
  endforeach()
  if(module STREQUAL Game)
    set(target Game)
    add_executable(${target} ${source_files})
  else()
    set(target s2_full_${module})
    add_library(${target} STATIC ${source_files})
  endif()
  target_compile_features(${target} PRIVATE cxx_std_17)
  target_compile_definitions(${target} PRIVATE S2_FULL_GAME S2_NATIVE_MUSIC S2_NATIVE_SFX S2_NATIVE_VIDEO LIFESTUDIOHEADAPI_EXPORTS_LIB NDEBUG)
  target_compile_options(${target} PRIVATE -include "${_s2_game_root}/Misc/GamePlatform.h")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${target} PRIVATE -fms-extensions -Wno-microsoft -Wno-dynamic-class-memaccess -Wno-inconsistent-missing-override -Wno-unknown-pragmas)
  endif()
  target_include_directories(${target} PRIVATE "${_s2_game_root}/${module}"
    "${_s2_game_root}/Main" "${_s2_game_root}/Misc" "${_s2_game_root}/FileIO"
    "${_s2_game_root}/third_party/fmod/include" "${_s2_game_root}/third_party/bink/include"
    "${_s2_game_root}/third_party/lifestudio/include" "${_s2_game_root}/third_party/lifestudio/src"
    "${_s2_game_root}/third_party/squish" "${CMAKE_BINARY_DIR}/bgfx-generated")
  target_link_libraries(${target} PRIVATE s2_platform bgfx)
endforeach()
target_link_libraries(s2_full_FileIO PUBLIC s2_portable_package s2_portable_structure s2_portable_user_paths)
target_link_libraries(s2_full_Main PUBLIC s2_portable_effects s2_vector_fonts s2_network_graph)
s2_copy_fonts(Game)
include("${root}/cmake/HDTextures.cmake")
s2_build_hd_textures(Game)
target_link_libraries(s2_full_ADOImport PUBLIC s2_portable_database)
add_dependencies(s2_full_Main S2BgfxShaders)
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
file(GLOB _s2_squish_sources "${root}/third_party/squish/squish/*.cpp")
add_library(s2_squish STATIC ${_s2_squish_sources})
target_include_directories(s2_squish PUBLIC "${root}/third_party/squish")
target_link_libraries(s2_full_Image PUBLIC PNG::PNG ZLIB::ZLIB s2_squish)
target_link_libraries(Game PRIVATE -Wl,--start-group
  -Wl,--whole-archive s2_full_Main s2_full_DBFormat -Wl,--no-whole-archive
  s2_full_ADOImport s2_full_FileIO s2_full_MiscDll s2_full_Misc s2_full_Input
  s2_full_Image s2_full_Script s2_full_FModSound
  s2_native_lifestudio s2_native_fmod s2_bink_compat -Wl,--end-group
  Threads::Threads ${CMAKE_DL_LIBS})

foreach(test IN ITEMS BgfxRendererTests SDLPlatformInputTests)
  add_executable(${test} "${_s2_game_root}/diagnostics/${test}.cpp")
  target_compile_features(${test} PRIVATE cxx_std_17)
  target_compile_definitions(${test} PRIVATE S2_FULL_GAME S2_NATIVE_MUSIC S2_NATIVE_SFX S2_NATIVE_VIDEO LIFESTUDIOHEADAPI_EXPORTS_LIB NDEBUG)
  target_compile_options(${test} PRIVATE -include "${_s2_game_root}/Misc/GamePlatform.h")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${test} PRIVATE -fms-extensions -Wno-microsoft)
  endif()
  target_include_directories(${test} PRIVATE "${_s2_game_root}/Main"
    "${_s2_game_root}/Misc" "${_s2_game_root}/FileIO"
    "${_s2_game_root}/third_party/lifestudio/include")
  target_link_libraries(${test} PRIVATE -Wl,--start-group
    -Wl,--whole-archive s2_full_Main s2_full_DBFormat -Wl,--no-whole-archive
    s2_full_ADOImport s2_full_FileIO s2_full_MiscDll s2_full_Misc s2_full_Input
    s2_full_Image s2_full_Script s2_full_FModSound
    s2_native_lifestudio s2_native_fmod s2_bink_compat -Wl,--end-group
    s2_platform bgfx Threads::Threads ${CMAKE_DL_LIBS})
  add_test(NAME ${test} COMMAND ${test})
endforeach()
add_test(NAME SDLPlatformInvalidDriver COMMAND SDLPlatformInputTests --invalid-driver)
if(S2_GAME_DIR AND EXISTS "${S2_GAME_DIR}/cfg/input.cfg")
  add_test(NAME SDLInputConfiguration
    COMMAND SDLPlatformInputTests --bindings "${S2_GAME_DIR}/cfg/input.cfg")
endif()

add_test(NAME DisplayRuntimeTests COMMAND BgfxRendererTests --display)
add_test(NAME FocusRestoreRuntimeTests COMMAND BgfxRendererTests --focus-restore)
set_tests_properties(FocusRestoreRuntimeTests PROPERTIES TIMEOUT 60)
add_test(NAME GraphicsOptionsRuntimeTests COMMAND BgfxRendererTests --graphics)
s2_copy_fonts(BgfxRendererTests)
