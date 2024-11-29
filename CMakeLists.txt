cmake_minimum_required(VERSION 2.20)
project(slcross CXX)

set(CMAKE_CXX_STANDARD 20)

include(cmake/FetchTint.cmake)
include(cmake/bundle.cmake)

add_subdirectory(${tint_SOURCE_DIR}/third_party/glslang/src ${CMAKE_CURRENT_BINARY_DIR}/glslang)
add_subdirectory(thirdparty/SPIRV-Cross)

add_library(slcross slcross.cpp)
target_link_libraries(slcross PRIVATE tint_api SPIRV-Tools-opt SPIRV-Tools-link glslang glslang::glslang-default-resource-limits spirv-cross-cpp spirv-cross-glsl spirv-cross-hlsl spirv-cross-msl)
target_include_directories(slcross PUBLIC .)
add_library(slcross::slcross ALIAS slcross)

# bundle_static_library(slcross slcross_bundled)
# add_library(slcross::bundled ALIAS slcross_bundled)