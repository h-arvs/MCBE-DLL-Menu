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
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
set(BUILD_SHARED_LIBS OFF)

file(GLOB_RECURSE sources "imgui/*.cpp")
add_library(MCBEDllMenu SHARED library.cpp ${sources})

CPMAddPackage("https://github.com/cursey/safetyhook.git#main")

set(KIERO_INCLUDE_D3D11 ON)
set(KIERO_INCLUDE_D3D12 ON)
set(KIERO_BUILD_EXAMPLES OFF)
CPMAddPackage("https://github.com/BasedInc/kiero.git#master")
target_compile_options(kiero PRIVATE /WX-) # stop treating warnings as errors

CPMAddPackage("https://gitlab.freedesktop.org/freetype/freetype.git#VER-2-14-1")
add_library(Freetype::Freetype ALIAS freetype)

set(RMLUI_CUSTOM_RTTI ON)
CPMAddPackage("https://github.com/JiayiSoftware/RmlUi.git#dx11-backend")
add_library(rmlui_backend INTERFACE)
target_sources(rmlui_backend INTERFACE
        "${RmlUi_SOURCE_DIR}/Backends/RmlUi_Renderer_DX11.cpp"
        "${RmlUi_SOURCE_DIR}/Backends/RmlUi_Renderer_DX11.h"
        "${RmlUi_SOURCE_DIR}/Backends/RmlUi_Platform_Win32.cpp"
        "${RmlUi_SOURCE_DIR}/Backends/RmlUi_Platform_Win32.h"
)
target_link_libraries(rmlui_backend INTERFACE d3d11.lib dxgi.lib d3dcompiler.lib)
target_include_directories(rmlui_backend INTERFACE "${RmlUi_SOURCE_DIR}/Backends")

CPMAddPackage("https://github.com/cameron314/readerwriterqueue.git#master")

target_link_libraries(MCBEDllMenu PRIVATE kiero safetyhook d3d11 GameInput rmlui_backend RmlUi::RmlUi readerwriterqueue)

