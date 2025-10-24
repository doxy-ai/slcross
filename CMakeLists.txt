cmake_minimum_required(VERSION 3.21)
project(slcross CXX)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag(-Wno-unused-but-set-variable HAVE_NO_UNUSED_FLAG)
if(HAVE_NO_UNUSED_FLAG)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-but-set-variable>)
endif()

set(SLANG_LIB_TYPE "STATIC" CACHE STRING "Slang library type")
set(SLANG_SLANG_LLVM_FLAVOR "DISABLE" CACHE STRING "Do not build llvm or fetch slang-llvm")

# set(SLANG_USE_SYSTEM_SPIRV_HEADERS ON CACHE BOOL "Build using SPIR-V headers located at a specific path")
# set(SLANG_SPIRV_HEADERS_INCLUDE_DIR "${slang_SOURCE_DIR}/external/spirv-headers/include" CACHE STRING "Provide a specific path for the SPIR-V headers and grammar files" FORCE)
# set(SLANG_ENABLE_SLANG_GLSLANG OFF CACHE BOOL "Disable unwanted slang dependency")

set(SLANG_ENABLE_GFX OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_SLANGD OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_SLANGC OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_TESTS OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_SLANG_RHI OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_REPLAYER OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_SLANGRT OFF CACHE BOOL "Disable unwanted slang component")

add_subdirectory(slang)

add_executable(tst hello.cpp)