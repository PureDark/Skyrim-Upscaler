function(add_cxx_files TARGET)
	file(GLOB_RECURSE INCLUDE_FILES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"include/*.h"
		"include/*.hpp"
		"include/*.hxx"
		"include/*.inl"
	)

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include
		PREFIX "Header Files"
		FILES ${INCLUDE_FILES})

	target_sources("${TARGET}" PUBLIC ${INCLUDE_FILES})

	file(GLOB_RECURSE HEADER_FILES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"src/*.h"
		"src/*.hpp"
		"src/*.hxx"
		"src/*.inl"
	)

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
		PREFIX "Header Files"
		FILES ${HEADER_FILES})

	target_sources("${TARGET}" PRIVATE ${HEADER_FILES})

	file(GLOB_RECURSE SOURCE_FILES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"src/*.cpp"
		"src/*.cxx"
	)

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
		PREFIX "Source Files"
		FILES ${SOURCE_FILES})

	target_sources("${TARGET}" PRIVATE ${SOURCE_FILES})
	
	file(GLOB_RECURSE SHADER_FILES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"src/hlsl/*.inc"
	)
	
	file(GLOB_RECURSE SHADER_VS_SOURCE_FILES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"src/hlsl/*_vs.hlsl"
	)
	
	file(GLOB_RECURSE SHADER_PS_SOURCE_FILES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"src/hlsl/*_ps.hlsl"
	)

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
		PREFIX "Shader Files"
		FILES ${SHADER_FILES}
		      ${SHADER_VS_SOURCE_FILES}
		      ${SHADER_PS_SOURCE_FILES})

	target_sources("${TARGET}" PRIVATE ${SHADER_FILES}
                                       ${SHADER_VS_SOURCE_FILES}
                                       ${SHADER_PS_SOURCE_FILES})

	set_source_files_properties(${SHADER_VS_SOURCE_FILES} PROPERTIES VS_SHADER_OBJECT_FILE_NAME "" VS_SHADER_OUTPUT_HEADER_FILE "../src/hlsl/%(Filename).inc" VS_SHADER_VARIABLE_NAME "%(Filename)" VS_SHADER_TYPE Vertex VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main)
	set_source_files_properties(${SHADER_PS_SOURCE_FILES} PROPERTIES VS_SHADER_OBJECT_FILE_NAME "" VS_SHADER_OUTPUT_HEADER_FILE "../src/hlsl/%(Filename).inc" VS_SHADER_VARIABLE_NAME "%(Filename)" VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main)
endfunction()
