include(FetchContent)

FetchContent_Declare(
	argparse

	# Manual download mode, even shallower than GIT_SHALLOW ON
	DOWNLOAD_COMMAND
		cd ${FETCHCONTENT_BASE_DIR}/argparse-src &&
		git init &&
		git fetch --depth=1 https://github.com/morrisfranken/argparse.git &&
		git reset --hard FETCH_HEAD &&
		git submodule update --depth 1 --init --recursive
)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(
	magic_enum

	# Manual download mode, even shallower than GIT_SHALLOW ON
	DOWNLOAD_COMMAND
		cd ${FETCHCONTENT_BASE_DIR}/magic_enum-src &&
		git init &&
		git fetch --depth=1 https://github.com/Neargye/magic_enum.git &&
		git reset --hard FETCH_HEAD &&
		git submodule update --depth 1 --init --recursive
)

FetchContent_GetProperties(magic_enum)
if (NOT magic_enum_POPULATED)
	FetchContent_Populate(magic_enum)

	add_library(slcross_magic_enum INTERFACE)
	target_include_directories(slcross_magic_enum INTERFACE ${magic_enum_SOURCE_DIR}/include/magic_enum)
endif()