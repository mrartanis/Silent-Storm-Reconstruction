option(S2_BUILD_MEDIA_PROBE "Build FFmpeg decoder probe for original media" OFF)
if(NOT S2_BUILD_MEDIA_PROBE)
  return()
endif()

set(S2_FFMPEG_ROOT "" CACHE PATH "Root of FFmpeg shared development package")
find_path(S2_FFMPEG_INCLUDE_DIR libavformat/avformat.h
  HINTS "${S2_FFMPEG_ROOT}/include")
if(NOT S2_FFMPEG_INCLUDE_DIR)
  message(FATAL_ERROR "FFmpeg development headers not found; set S2_FFMPEG_ROOT")
endif()
set(_ffmpeg_libs "")
foreach(component IN ITEMS avformat avcodec avutil swscale swresample)
  find_library(S2_FFMPEG_${component}_LIB NAMES ${component}
    HINTS "${S2_FFMPEG_ROOT}/lib")
  if(NOT S2_FFMPEG_${component}_LIB)
    message(FATAL_ERROR "FFmpeg ${component} library not found in S2_FFMPEG_ROOT")
  endif()
  list(APPEND _ffmpeg_libs "${S2_FFMPEG_${component}_LIB}")
endforeach()
add_library(s2_ffmpeg_decoder STATIC "${root}/Media/FFmpegDecoder.cpp")
target_include_directories(s2_ffmpeg_decoder PUBLIC "${root}/Media"
  PRIVATE "${S2_FFMPEG_INCLUDE_DIR}")
target_compile_features(s2_ffmpeg_decoder PRIVATE cxx_std_17)
target_link_libraries(s2_ffmpeg_decoder PUBLIC ${_ffmpeg_libs})
add_executable(FFmpegMediaProbe "${root}/diagnostics/FFmpegMediaProbe.cpp")
target_link_libraries(FFmpegMediaProbe PRIVATE s2_ffmpeg_decoder)

option(S2_BUILD_MINIAUDIO_MUSIC_PROBE "Build decoded-music/miniaudio mixer probe" OFF)
if(S2_BUILD_MINIAUDIO_MUSIC_PROBE)
  set(S2_MINIAUDIO_INCLUDE_DIR "" CACHE PATH "Directory containing pinned miniaudio.h")
  if(NOT EXISTS "${S2_MINIAUDIO_INCLUDE_DIR}/miniaudio.h")
    message(FATAL_ERROR "Set S2_MINIAUDIO_INCLUDE_DIR to miniaudio 0.11.25")
  endif()
  add_library(s2_native_music STATIC
    "${root}/Media/MiniaudioImpl.cpp"
    "${root}/Media/NativeMusicPlayer.cpp")
  target_include_directories(s2_native_music PRIVATE
    "${S2_MINIAUDIO_INCLUDE_DIR}")
  target_compile_features(s2_native_music PRIVATE cxx_std_17)
  target_link_libraries(s2_native_music PUBLIC s2_ffmpeg_decoder)
  add_executable(MusicMixerProbe "${root}/diagnostics/MusicMixerProbe.cpp")
  target_link_libraries(MusicMixerProbe PRIVATE s2_native_music)
endif()
