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
	FetchContent_Populate(slang)

	set(SLANG_LIB_TYPE STATIC)
	set(SLANG_SLANG_LLVM_FLAVOR DISABLE)

	set(SLANG_USE_SYSTEM_SPIRV_HEADERS ON)
	set(SLANG_SPIRV_HEADERS_INCLUDE_DIR "${slang_SOURCE_DIR}/external/spirv-headers/include")
	set(SLANG_ENABLE_SLANG_GLSLANG OFF)

	set(SLANG_ENABLE_GFX OFF)
	set(SLANG_ENABLE_SLANGD OFF)
	set(SLANG_ENABLE_SLANGC OFF)
	set(SLANG_ENABLE_TESTS OFF)
	set(SLANG_ENABLE_EXAMPLES OFF)
	set(SLANG_ENABLE_SLANG_RHI OFF)

	# Disable install
	macro (install)
	endmacro ()

	add_subdirectory(${slang_SOURCE_DIR} ${slang_BINARY_DIR} EXCLUDE_FROM_ALL)

	# Renable install
	macro (install)
		_install(${ARGV})
	endmacro(install)
endif()