# This file is part of the "Learn WebGPU for C++" book.
#   https://eliemichel.github.io/LearnWebGPU
#
# MIT License
# Copyright (c) 2022-2024 Elie Michel and the wgpu-native authors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Prevent multiple includes
if (TARGET dawn_native)
	return()
endif()

include(FetchContent)

FetchContent_Declare(
	dawn

	# Manual download mode, even shallower than GIT_SHALLOW ON
	DOWNLOAD_COMMAND
		cd ${FETCHCONTENT_BASE_DIR}/dawn-src &&
		git init &&
		git fetch --depth=1 https://dawn.googlesource.com/dawn chromium/6719 &&
		git reset --hard FETCH_HEAD

	# PATCH_COMMAND
	# 	git apply --ignore-space-change --ignore-whitespace "${CMAKE_CURRENT_LIST_DIR}/../patch/dawn.patch"
)

FetchContent_GetProperties(dawn)
if (NOT dawn_POPULATED)
	FetchContent_Populate(dawn)

	# This option replaces depot_tools
	set(DAWN_FETCH_DEPENDENCIES ON)

	if(WIN32)
		set(USE_VULKAN OFF)
		set(USE_METAL OFF)
		set(USE_DX12 ON)
	elseif(APPLE)
		set(USE_VULKAN OFF)
		set(USE_METAL ON)
		set(USE_DX12 OFF)
	else()
		set(USE_VULKAN ON)
		set(USE_METAL OFF)
		set(USE_DX12 OFF)
	endif()

	set(DAWN_ENABLE_D3D11 OFF)
	set(DAWN_ENABLE_D3D12 ${USE_DX12})
	set(DAWN_ENABLE_METAL ${USE_METAL})
	set(DAWN_ENABLE_NULL OFF)
	set(DAWN_ENABLE_DESKTOP_GL OFF)
	set(DAWN_ENABLE_OPENGLES OFF)
	set(DAWN_ENABLE_VULKAN ${USE_VULKAN})
	set(DAWN_ENABLE_GLFW OFF)

	set(TINT_BUILD_SPV_WRITER ON)
	set(TINT_BUILD_SPV_READER ON)
	set(TINT_BUILD_GLSL_WRITER ON)
	set(TINT_BUILD_HLSL_WRITER ON)
	set(TINT_BUILD_MSL_WRITER ON)


	# Disable unneeded parts
	set(DAWN_BUILD_SAMPLES OFF)
	set(TINT_BUILD_CMD_TOOLS OFF)
	set(TINT_BUILD_TESTS OFF)
	set(TINT_BUILD_IR_BINARY OFF)

	add_subdirectory(${dawn_SOURCE_DIR} ${dawn_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

set(AllDawnTargets
	core_tables
	dawn_common
	dawn_glfw
	dawn_headers
	dawn_native
	dawn_platform
	dawn_proc
	dawn_wire
	dawn_native_objects
	dawn_shared_utils
	partition_alloc
	dawncpp
	dawncpp_headers
	enum_string_mapping
	extinst_tables
	webgpu_dawn
	webgpu_headers_gen

	tint-format
	tint-lint
	tint_api
	tint_api_common
	tint_cmd_common
	tint_lang_core
	tint_lang_core_common
	tint_lang_core_constant
	tint_lang_core_intrinsic
	tint_lang_core_ir
	tint_lang_core_ir_transform
	tint_lang_core_type
	tint_lang_glsl
	tint_lang_glsl_intrinsic
	tint_lang_glsl_ir
	tint_lang_glsl_validate
	tint_lang_glsl_writer
	tint_lang_glsl_writer_common
	tint_lang_glsl_writer_helpers
	tint_lang_glsl_writer_printer
	tint_lang_glsl_writer_raise
	tint_lang_hlsl
	tint_lang_hlsl_intrinsic
	tint_lang_hlsl_ir
	tint_lang_hlsl_type
	tint_lang_hlsl_validate
	tint_lang_hlsl_writer
	tint_lang_hlsl_writer_ast_printer
	tint_lang_hlsl_writer_ast_raise
	tint_lang_hlsl_writer_common
	tint_lang_hlsl_writer_helpers
	tint_lang_hlsl_writer_printer
	tint_lang_hlsl_writer_raise
	tint_lang_msl
	tint_lang_msl_intrinsic
	tint_lang_msl_ir
	tint_lang_msl_type
	tint_lang_msl_validate
	tint_lang_msl_writer
	tint_lang_msl_writer_ast_printer
	tint_lang_msl_writer_ast_printer
	tint_lang_msl_writer_ast_raise
	tint_lang_msl_writer_ast_raise
	tint_lang_msl_writer_common
	tint_lang_msl_writer_helpers
	tint_lang_spirv
	tint_lang_spirv_intrinsic
	tint_lang_spirv_ir
	tint_lang_spirv_reader
	tint_lang_spirv_reader_ast_lower
	tint_lang_spirv_reader_ast_parser
	tint_lang_spirv_reader_common
	tint_lang_spirv_reader_lower
	tint_lang_spirv_reader_parser
	tint_lang_spirv_type
	tint_lang_spirv_validate
	tint_lang_spirv_writer
	tint_lang_spirv_writer_common
	tint_lang_spirv_writer_helpers
	tint_lang_spirv_writer_printer
	tint_lang_spirv_writer_raise
	tint_lang_wgsl
	tint_lang_wgsl_ast
	tint_lang_wgsl_ast_transform
	tint_lang_wgsl_common
	tint_lang_wgsl_features
	tint_lang_wgsl_helpers
	tint_lang_wgsl_inspector
	tint_lang_wgsl_intrinsic
	tint_lang_wgsl_ir
	tint_lang_wgsl_program
	tint_lang_wgsl_reader
	tint_lang_wgsl_reader_lower
	tint_lang_wgsl_reader_parser
	tint_lang_wgsl_reader_program_to_ir
	tint_lang_wgsl_resolver
	tint_lang_wgsl_sem
	tint_lang_wgsl_writer
	tint_lang_wgsl_writer_ast_printer
	tint_lang_wgsl_writer_ir_to_program
	tint_lang_wgsl_writer_raise
	tint_lang_wgsl_writer_syntax_tree_printer
	tint_utils_bytes
	tint_utils_cli
	tint_utils_command
	tint_utils_constants
	tint_utils_containers
	tint_utils_debug
	tint_utils_diagnostic
	tint_utils_file
	tint_utils_generator
	tint_utils_ice
	tint_utils_id
	tint_utils_macros
	tint_utils_math
	tint_utils_memory
	tint_utils_reflection
	tint_utils_result
	tint_utils_rtti
	tint_utils_socket
	tint_utils_strconv
	tint_utils_symbol
	tint_utils_system
	tint_utils_text
	tint_utils_traits
)

foreach (Target ${AllDawnTargets})
	if (TARGET ${Target})
		# Is a target...
		get_property(AliasedTarget TARGET "${Target}" PROPERTY ALIASED_TARGET)
		if ("${AliasedTarget}" STREQUAL "")
			# ...and is not an alias -> move to the Dawn folder
			set_property(TARGET ${Target} PROPERTY FOLDER "Dawn")
		endif()
	else()
		message(STATUS "NB: '${Target}' is no longer a target of the Dawn project.")
	endif()
endforeach()