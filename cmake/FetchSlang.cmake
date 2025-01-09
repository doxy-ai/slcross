include(FetchContent)

FetchContent_Declare(
	slang

	# Manual download mode, even shallower than GIT_SHALLOW ON
	DOWNLOAD_COMMAND
		cd ${FETCHCONTENT_BASE_DIR}/slang-src &&
		git init &&
		git fetch --depth=1 https://github.com/shader-slang/slang.git v2024.17 &&
		git reset --hard FETCH_HEAD &&
		git submodule update --depth 1 --init --recursive
)

FetchContent_GetProperties(slang)
if (NOT slang_POPULATED)
	set(SLANG_EMBED_CORE_MODULE OFF CACHE BOOL "Build slang with an embedded version of the core module")
	set(SLANG_LIB_TYPE "STATIC" CACHE STRING "Slang library type")
	set(SLANG_SLANG_LLVM_FLAVOR "DISABLE" CACHE STRING "Do not build llvm or fetch slang-llvm")

	set(SLANG_USE_SYSTEM_SPIRV_HEADERS ON)
	set(SLANG_SPIRV_HEADERS_INCLUDE_DIR "${slang_SOURCE_DIR}/external/spirv-headers/include")
	set(SLANG_ENABLE_SLANG_GLSLANG OFF)

	set(SLANG_ENABLE_GFX OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_SLANGD OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_SLANGC OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_TESTS OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_SLANG_RHI OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_REPLAYER OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_SLANGRT OFF CACHE BOOL "Disable unwanted slang component")

	FetchContent_Populate(slang)

	# Disable install
	macro (install)
	endmacro ()

	add_subdirectory(${slang_SOURCE_DIR} ${slang_BINARY_DIR} EXCLUDE_FROM_ALL)

	# Renable install
	macro (install)
		_install(${ARGV})
	endmacro(install)
endif()