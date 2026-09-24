cmake_minimum_required(VERSION 3.21)
project(S2SDL3BgfxProbe LANGUAGES C CXX)

set(S2_BGFX_CMAKE_ROOT "" CACHE PATH "Pinned bgfx.cmake checkout with bgfx, bimg and bx submodules")
set(S2_BGFX_DIR "${S2_BGFX_CMAKE_ROOT}/bgfx" CACHE PATH "bgfx source tree")
set(S2_BIMG_DIR "${S2_BGFX_CMAKE_ROOT}/bimg" CACHE PATH "bimg source tree")
set(S2_BX_DIR "${S2_BGFX_CMAKE_ROOT}/bx" CACHE PATH "bx source tree")
if(NOT EXISTS "${S2_BGFX_CMAKE_ROOT}/CMakeLists.txt" OR
   NOT EXISTS "${S2_BGFX_DIR}/include/bgfx/bgfx.h" OR
   NOT EXISTS "${S2_BIMG_DIR}/include/bimg/bimg.h" OR
   NOT EXISTS "${S2_BX_DIR}/include/bx/bx.h")
  message(FATAL_ERROR "Set S2_BGFX_CMAKE_ROOT, S2_BGFX_DIR, S2_BIMG_DIR and S2_BX_DIR to complete pinned source trees")
endif()

find_package(SDL3 CONFIG REQUIRED)
set(BGFX_DIR "${S2_BGFX_DIR}" CACHE PATH "" FORCE)
set(BIMG_DIR "${S2_BIMG_DIR}" CACHE PATH "" FORCE)
set(BX_DIR "${S2_BX_DIR}" CACHE PATH "" FORCE)
set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
add_subdirectory("${S2_BGFX_CMAKE_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/bgfx")

add_executable(S2SDL3BgfxProbe main.cpp)
target_compile_features(S2SDL3BgfxProbe PRIVATE cxx_std_20)
target_link_libraries(S2SDL3BgfxProbe PRIVATE SDL3::SDL3 bgfx)
