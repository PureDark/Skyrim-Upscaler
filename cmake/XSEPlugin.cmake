add_compile_definitions(SKYRIM)
set(CommonLibPath "extern/CommonLibSSE-NG")
set(CommonLibName "CommonLibSSE")

add_library("${PROJECT_NAME}" SHARED)

target_compile_features(
	"${PROJECT_NAME}"
	PRIVATE
	cxx_std_23
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(AddCXXFiles)
add_cxx_files("${PROJECT_NAME}")

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/Plugin.h.in
	${CMAKE_CURRENT_BINARY_DIR}/cmake/Plugin.h
	@ONLY
)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
	${CMAKE_CURRENT_BINARY_DIR}/cmake/version.rc
	@ONLY
)

target_sources(
	"${PROJECT_NAME}"
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/cmake/Plugin.h
		${CMAKE_CURRENT_BINARY_DIR}/cmake/version.rc
		.clang-format
		.editorconfig)

target_precompile_headers(
	"${PROJECT_NAME}"
	PRIVATE
		include/PCH.h
)

find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")

target_include_directories(
	"${PROJECT_NAME}"
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/extern/nvapi
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/cmake
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${SIMPLEINI_INCLUDE_DIRS}
)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

if (CMAKE_GENERATOR MATCHES "Visual Studio")
	add_compile_definitions(_UNICODE)

	target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:DEBUG>")

	set(SC_RELEASE_OPTS "/Zi;/fp:fast;/GL;/Gy-;/Gm-;/Gw;/sdl-;/GS-;/guard:cf-;/O2;/Ob2;/Oi;/Ot;/Oy;/fp:except-")	
	
	target_compile_options(
		"${PROJECT_NAME}"
		PRIVATE
			/MP
			/await
			/W4
			/WX-
			/permissive-
			/Zc:alignedNew
			/Zc:auto
			/Zc:__cplusplus
			/Zc:externC
			/Zc:externConstexpr
			/Zc:forScope
			/Zc:hiddenFriend
			/Zc:implicitNoexcept
			/Zc:lambda
			/Zc:noexceptTypes
			/Zc:preprocessor
			/Zc:referenceBinding
			/Zc:rvalueCast
			/Zc:sizedDealloc
			/Zc:strictStrings
			/Zc:ternary
			/Zc:threadSafeInit
			/Zc:trigraphs
			/Zc:wchar_t
			/wd4200 # nonstandard extension used : zero-sized array in struct/union
			/arch:AVX
			/utf-8
	)

	target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:/fp:strict>")
	target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:/ZI>")
	target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:/Od>")
	target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:/Gy>")
	target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${SC_RELEASE_OPTS}>")

	target_link_options(
		${PROJECT_NAME}
		PRIVATE
			/WX
			"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
			"$<$<CONFIG:RELEASE>:/LTCG;/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
	)

	set(list_of_dlls PDPerfPlugin.dll)
	foreach(dll_name ${list_of_dlls})
	   set(space_separated_list "${space_separated_list} /DELAYLOAD:${dll_name}")
	endforeach()
	set_target_properties(${PROJECT_NAME}  PROPERTIES LINK_FLAGS "${space_separated_list}")
endif()

add_subdirectory(${CommonLibPath} ${CommonLibName} EXCLUDE_FROM_ALL)

find_package(spdlog CONFIG REQUIRED)

target_include_directories(
	${PROJECT_NAME}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
		imgui::imgui
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/cmake
		${CMAKE_CURRENT_SOURCE_DIR}/src
		debug ${CMAKE_CURRENT_SOURCE_DIR}/extern/detours/x64/Debug/detours.lib
		optimized ${CMAKE_CURRENT_SOURCE_DIR}/extern/detours/x64/Release/detours.lib
		debug ${CMAKE_CURRENT_SOURCE_DIR}/extern/PDPerfPlugin.lib
		optimized ${CMAKE_CURRENT_SOURCE_DIR}/extern/PDPerfPlugin.lib
		debug ${CMAKE_CURRENT_SOURCE_DIR}/extern/nvapi/amd64/nvapi64.lib
		optimized ${CMAKE_CURRENT_SOURCE_DIR}/extern/nvapi/amd64/nvapi64.lib
)

target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
	CommonLibSSE::CommonLibSSE
)