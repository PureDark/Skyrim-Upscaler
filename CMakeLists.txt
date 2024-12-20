cmake_minimum_required(VERSION 3.20)

project(
	SkyrimUpscaler
	VERSION 1.0.0
	LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(XSEPlugin)

find_path(BSHOSHANY_THREAD_POOL_INCLUDE_DIRS "BS_thread_pool.hpp")
find_package(magic_enum CONFIG REQUIRED)
find_package(xbyak CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(EASTL CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)
find_path(CLIB_UTIL_INCLUDE_DIRS "ClibUtil/utils.hpp")
find_package(pystring CONFIG REQUIRED)
find_package(cppwinrt CONFIG REQUIRED)
find_package(unordered_dense CONFIG REQUIRED)
find_package(efsw CONFIG REQUIRED)
find_package(Tracy CONFIG REQUIRED)

target_compile_definitions(
	${PROJECT_NAME}
	PRIVATE
	"$<$<BOOL:${TRACY_SUPPORT}>:TRACY_SUPPORT>"
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
	${BSHOSHANY_THREAD_POOL_INCLUDE_DIRS}
	${CLIB_UTIL_INCLUDE_DIRS}
)

target_link_libraries(
	${PROJECT_NAME}
	PRIVATE
	debug ${CMAKE_CURRENT_SOURCE_DIR}/include/detours/Debug/detours.lib
	optimized ${CMAKE_CURRENT_SOURCE_DIR}/include/detours/Release/detours.lib
	Microsoft::CppWinRT
	magic_enum::magic_enum
	xbyak::xbyak
	nlohmann_json::nlohmann_json
	imgui::imgui
	EASTL
	Microsoft::DirectXTK
	Microsoft::DirectXTex
	pystring::pystring
	unordered_dense::unordered_dense
	efsw::efsw
	Tracy::TracyClient
)