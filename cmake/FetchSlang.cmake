include(FetchContent)

FetchContent_Declare(
	slang

	# Manual download mode, even shallower than GIT_SHALLOW ON
	DOWNLOAD_COMMAND
		cd ${FETCHCONTENT_BASE_DIR}/slang-src &&
		git init &&
		git fetch --depth=1 https://github.com/shader-slang/slang.git v2025.13.1 &&
		git reset --hard FETCH_HEAD &&
		git submodule update --depth 1 --init --recursive

	# Need to apply a patch so that slang uses its own SPIRV headers instead of tint's
	PATCH_COMMAND
		git apply --ignore-space-change --ignore-whitespace "${CMAKE_CURRENT_SOURCE_DIR}/patches/slang.patch"
)

FetchContent_GetProperties(slang)
if (NOT slang_POPULATED)
	# set(SLANG_EMBED_CORE_MODULE OFF CACHE BOOL "Build slang with an embedded version of the core module")
	set(SLANG_LIB_TYPE "STATIC" CACHE STRING "Slang library type")
	set(SLANG_SLANG_LLVM_FLAVOR "DISABLE" CACHE STRING "Do not build llvm or fetch slang-llvm")

	set(SLANG_USE_SYSTEM_SPIRV_HEADERS ON CACHE BOOL "Build using SPIR-V headers located at a specific path")
	set(SLANG_SPIRV_HEADERS_INCLUDE_DIR "${slang_SOURCE_DIR}/external/spirv-headers/include" CACHE STRING "Provide a specific path for the SPIR-V headers and grammar files" FORCE)
	set(SLANG_ENABLE_SLANG_GLSLANG OFF CACHE BOOL "Disable unwanted slang dependency")

	set(SLANG_ENABLE_GFX OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_SLANGD OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_SLANGC OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_TESTS OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_SLANG_RHI OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_REPLAYER OFF CACHE BOOL "Disable unwanted slang component")
	set(SLANG_ENABLE_SLANGRT OFF CACHE BOOL "Disable unwanted slang component")

	FetchContent_Populate(slang)

	# # Small patch to use the SPIRV-Headers shipped with slang instead of the ones shipped with dawn
	# set(FILE_PATH "${slang_SOURCE_DIR}/source/compiler-core/slang-spirv-core-grammar.h")
	# file(READ ${FILE_PATH} FILE_CONTENTS)
	# string(REPLACE "#include <spirv/unified1/spirv.h>" "#include \"../../external/spirv-headers/include/spirv/unified1/spirv.h\"" NEW_FILE_CONTENTS "${FILE_CONTENTS}")
	# file(WRITE ${FILE_PATH} "${NEW_FILE_CONTENTS}")

	# # TODO: Why is this patch necessary?
	# set(FILE_PATH "${slang_SOURCE_DIR}/source/core/slang-hash.h")
	# file(READ ${FILE_PATH} FILE_CONTENTS)
	# string(REPLACE "#include <ankerl/unordered_dense.h>" "#include \"../external/unordered_dense/include/ankerl/unordered_dense.h\"" NEW_FILE_CONTENTS "${FILE_CONTENTS}")
	# file(WRITE ${FILE_PATH} "${NEW_FILE_CONTENTS}")

	# Disable install
	macro (install)
	endmacro ()

	add_subdirectory(${slang_SOURCE_DIR} ${slang_BINARY_DIR} EXCLUDE_FROM_ALL)

	# Renable install
	macro (install)
		_install(${ARGV})
	endmacro(install)
endif()