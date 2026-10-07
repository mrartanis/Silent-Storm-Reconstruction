# Build the offline visual overlay for the host, including cross-compiled games.
function(s2_build_hd_textures target)
  option(S2_BUILD_HD_TEXTURES "Build the offline HD texture pack beside Game" ON)
  if(NOT S2_BUILD_HD_TEXTURES)
    return()
  endif()
  find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
  execute_process(COMMAND "${Python3_EXECUTABLE}" -c "from PIL import Image"
    RESULT_VARIABLE _s2_pillow_result ERROR_VARIABLE _s2_pillow_error)
  if(NOT _s2_pillow_result EQUAL 0)
    message(FATAL_ERROR "HD textures need Pillow for ${Python3_EXECUTABLE}. Install it with: ${Python3_EXECUTABLE} -m pip install Pillow. ${_s2_pillow_error}")
  endif()
  add_custom_target(S2HDTextures
    COMMAND "${Python3_EXECUTABLE}" "${root}/assets/terrain-hd/build_pack.py"
      --output "$<TARGET_FILE_DIR:${target}>/res-hd" --incremental
    COMMENT "Building offline HD world textures"
    VERBATIM)
  add_dependencies(${target} S2HDTextures)
endfunction()
