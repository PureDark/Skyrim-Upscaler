option(BUILD_SKYRIM "Build for Skyrim" OFF)
option(BUILD_SKYRIMVR "Build for Skyrim VR" OFF)
option(BUILD_FALLOUT4 "Build for Fallout 4" OFF)

if(BUILD_SKYRIM)
	add_compile_definitions(SKYRIM)
	set(CommonLibName "CommonLibSSE")
	set(GameVersion "Skyrim")
elseif(BUILD_SKYRIMVR)
	add_compile_definitions(SKYRIMVR)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	set(CommonLibName "CommonLibVR")
	set(GameVersion "Skyrim VR")
elseif(BUILD_FALLOUT4)
	add_compile_definitions(FALLOUT4)
	set(CommonLibPath "CommonLibF4/CommonLibF4")
	set(CommonLibName "external/CommonLibF4")
	set(GameVersion "Fallout 4")
else()
	message(
	FATAL_ERROR
		"A game must be selected."
	)
endif()

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
	# add_compile_definitions(_UNICODE)

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

find_package(nlohmann_json CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

if (BUILD_SKYRIM)
	find_package(CommonLibSSE REQUIRED)
	target_link_libraries(
		${PROJECT_NAME} 
		PUBLIC 
			CommonLibSSE::CommonLibSSE
			imgui::imgui
		PRIVATE
			magic_enum::magic_enum
			debug ${CMAKE_CURRENT_SOURCE_DIR}/extern/detours/x64/Debug/detours.lib
			optimized ${CMAKE_CURRENT_SOURCE_DIR}/extern/detours/x64/Release/detours.lib
			debug ${CMAKE_CURRENT_SOURCE_DIR}/extern/PDPerfPlugin.lib
			optimized ${CMAKE_CURRENT_SOURCE_DIR}/extern/PDPerfPlugin.lib
	)
else()
	add_subdirectory(${CommonLibPath} ${CommonLibName} EXCLUDE_FROM_ALL)
endif()

find_package(spdlog CONFIG REQUIRED)
