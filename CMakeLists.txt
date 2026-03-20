file(
        DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
        EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)
set(CPM_SOURCE_CACHE ${CMAKE_SOURCE_DIR}/cpm-cache)

cmake_minimum_required(VERSION 3.5)
project(MCBEDllMenu)
set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE sources "imgui/*.cpp")
add_library(MCBEDllMenu SHARED library.cpp ${sources})

CPMAddPackage("https://github.com/cursey/safetyhook.git#main")

set(KIERO_INCLUDE_D3D11 ON)
set(KIERO_INCLUDE_D3D12 ON)
set(KIERO_BUILD_EXAMPLES OFF)
CPMAddPackage("https://github.com/BasedInc/kiero.git#master")
target_compile_options(kiero PRIVATE /WX-) # stop treating warnings as errors

target_link_libraries(MCBEDllMenu PRIVATE kiero safetyhook d3d11 GameInput)

