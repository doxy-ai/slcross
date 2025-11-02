cmake_minimum_required(VERSION 3.12)
project(slcross CXX)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag(-Wno-unused-but-set-variable HAVE_NO_UNUSED_FLAG)
if(HAVE_NO_UNUSED_FLAG)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-but-set-variable>)
endif()

check_cxx_compiler_flag(/Wv:18 HAVE_V18)
if(HAVE_V18)
  add_compile_options(/Wv:18)
endif()

set(SLANG_LIB_TYPE "STATIC" CACHE STRING "Slang library type")
set(SLANG_SLANG_LLVM_FLAVOR "DISABLE" CACHE STRING "Do not build llvm or fetch slang-llvm")

# set(SLANG_USE_SYSTEM_SPIRV_HEADERS ON CACHE BOOL "Build using SPIR-V headers located at a specific path")
# set(SLANG_SPIRV_HEADERS_INCLUDE_DIR "${slang_SOURCE_DIR}/external/spirv-headers/include" CACHE STRING "Provide a specific path for the SPIR-V headers and grammar files" FORCE)
# set(SLANG_ENABLE_SLANG_GLSLANG OFF CACHE BOOL "Disable unwanted slang dependency")

set(SLANG_ENABLE_GFX OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_SLANGD OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_SLANGC OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_SLANGI OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_TESTS OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_SLANG_RHI OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_REPLAYER OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_ENABLE_SLANGRT OFF CACHE BOOL "Disable unwanted slang component")

set(SLANG_ENABLE_DXIL OFF CACHE BOOL "Disable unwanted slang component")
set(SLANG_EXCLUDE_DAWN ON CACHE BOOL "Disable unwanted slang component")

set(SPIRV_CROSS_CLI OFF CACHE BOOL "Disable unwanted spirv-cross component")
set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "Disable unwanted spirv-cross component")
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "Disable unwanted spirv-cross component")
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "Disable unwanted spirv-cross component")
set(SPIRV_CROSS_ENABLE_UTIL OFF CACHE BOOL "Disable unwanted spirv-cross component")

# Patch the slang-session.cpp file to change a faulty include line
set(SLANG_SESSION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/slang/source/slang/slang-session.cpp")
set(OLD_INCLUDE_LINE "#include \"slang-standard-module-config.h\"")
set(NEW_INCLUDE_LINE "#include \"${CMAKE_CURRENT_BINARY_DIR}/thirdparty/slang/source/standard-modules/slang-standard-module-config-header/slang-standard-module-config.h\"")
# Calculate the MD5 hash of the original file content
file(MD5 ${SLANG_SESSION_FILE} SLANG_SESSION_FILE_HASH)
if(EXISTS "${CMAKE_BINARY_DIR}/slang-session-hash.txt")
  file(READ "${CMAKE_BINARY_DIR}/slang-session-hash.txt" SLANG_SESSION_OLD_HASH)
else()
  set(SLANG_SESSION_OLD_HASH "")
endif()
if(NOT SLANG_SESSION_FILE_HASH STREQUAL SLANG_SESSION_OLD_HASH)
  file(READ ${SLANG_SESSION_FILE} SLANG_SESSION_CONTENT)
  string(REPLACE "${OLD_INCLUDE_LINE}" "${NEW_INCLUDE_LINE}" SLANG_SESSION_CONTENT "${SLANG_SESSION_CONTENT}")
  file(WRITE ${SLANG_SESSION_FILE} "${SLANG_SESSION_CONTENT}")
  file(WRITE "${CMAKE_BINARY_DIR}/slang-session-hash.txt" "${SLANG_SESSION_FILE_HASH}")
endif()

add_subdirectory(thirdparty/slang EXCLUDE_FROM_ALL)
add_subdirectory(thirdparty/SPIRV-Cross EXCLUDE_FROM_ALL)
add_subdirectory(thirdparty/magic_enum EXCLUDE_FROM_ALL)

add_library(slcross slcross.cpp)
target_link_libraries(slcross PUBLIC slang spirv-cross-hlsl magic_enum)
target_include_directories(slcross PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(slcross PROPERTIES CXX_STANDARD 20)

# add_executable(tst hello.cpp)
# target_link_libraries(tst PRIVATE SPIRV-Tools-opt SPIRV-Tools-link slcross)
# set_target_properties(tst PROPERTIES CXX_STANDARD 20)