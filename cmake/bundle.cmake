# from: https://stackoverflow.com/a/66575545
function(bundle_static_library tgt_name bundled_tgt_name)
	list(APPEND static_libs ${tgt_name})

	function(_recursively_collect_dependencies input_target)
		set(_input_link_libraries LINK_LIBRARIES)
		get_target_property(_input_type ${input_target} TYPE)
		if (${_input_type} STREQUAL "INTERFACE_LIBRARY")
			set(_input_link_libraries INTERFACE_LINK_LIBRARIES)
		endif()
		get_target_property(public_dependencies ${input_target} ${_input_link_libraries})
		foreach(dependency IN LISTS public_dependencies)
			if(TARGET ${dependency})
				get_target_property(alias ${dependency} ALIASED_TARGET)
				if (TARGET ${alias})
					set(dependency ${alias})
				endif()
				get_target_property(_type ${dependency} TYPE)
				if (${_type} STREQUAL "STATIC_LIBRARY")
					list(APPEND static_libs ${dependency})
				endif()

				get_property(library_already_added
					GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency})
				if (NOT library_already_added)
					set_property(GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency} ON)
					_recursively_collect_dependencies(${dependency})
				endif()
			endif()
		endforeach()
		set(static_libs ${static_libs} PARENT_SCOPE)
	endfunction()

	_recursively_collect_dependencies(${tgt_name})

	list(REMOVE_DUPLICATES static_libs)

	set(bundled_tgt_full_name
		${CMAKE_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundled_tgt_name}${CMAKE_STATIC_LIBRARY_SUFFIX})

	if (CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$")
		file(WRITE ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
			"CREATE ${bundled_tgt_full_name}\n" )

		foreach(tgt IN LISTS static_libs)
			file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
				"ADDLIB $<TARGET_FILE:${tgt}>\n")
		endforeach()

		file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "SAVE\n")
		file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "END\n")

		file(GENERATE
			OUTPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
			INPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in)

		set(ar_tool ${CMAKE_AR})
		if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
			set(ar_tool ${CMAKE_CXX_COMPILER_AR})
		endif()

		add_custom_command(
			COMMAND ${ar_tool} -M < ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
			DEPENDS ${static_libs}
			OUTPUT ${bundled_tgt_full_name}
			COMMENT "Bundling ${bundled_tgt_name}"
			VERBATIM)
	elseif(MSVC)
		find_program(lib_tool lib)

		foreach(tgt IN LISTS static_libs)
			list(APPEND static_libs_full_names $<TARGET_FILE:${tgt}>)
		endforeach()

		add_custom_command(
			COMMAND ${lib_tool} /NOLOGO /OUT:${bundled_tgt_full_name} ${static_libs_full_names}
			DEPENDS ${static_libs}
			OUTPUT ${bundled_tgt_full_name}
			COMMENT "Bundling ${bundled_tgt_name}"
			VERBATIM)
	else()
		message(FATAL_ERROR "Unknown bundle scenario!")
	endif()

	set(bundling_target ${bundled_tgt_name}_bundling_target)
	add_custom_target(${bundling_target} ALL DEPENDS ${bundled_tgt_full_name})
	add_dependencies(${bundling_target} ${tgt_name})

	add_library(${bundled_tgt_name} STATIC IMPORTED)
	set_target_properties(${bundled_tgt_name}
		PROPERTIES
			IMPORTED_LOCATION ${bundled_tgt_full_name}
			INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${tgt_name},INTERFACE_INCLUDE_DIRECTORIES>)
	add_dependencies(${bundled_tgt_name} ${bundling_target})

endfunction()