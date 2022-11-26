#pragma once

//#undef NDEBUG

#pragma warning(push)
#pragma warning(disable: 5105)
#pragma warning(disable: 4189)
#pragma warning(disable: 4267)

#if defined(FALLOUT4)
#	include "F4SE/F4SE.h"
#	include "RE/Fallout.h"
#	define SKSE F4SE
#	define SKSEAPI F4SEAPI
#	define SKSEPlugin_Load F4SEPlugin_Load
#	define SKSEPlugin_Query F4SEPlugin_Query
#	define RUNTIME RUNTIME_1_10_163
#else
#	include "RE/Skyrim.h"
#	include "SKSE/SKSE.h"
#endif

#pragma warning(pop)

#include <Windows.h>

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif

using namespace std::literals;

namespace stl
{
	using namespace SKSE::stl;

	template <class T>
	void write_thunk_call(std::uintptr_t a_src)
	{
		SKSE::AllocTrampoline(14);

		auto& trampoline = SKSE::GetTrampoline();
		T::func = trampoline.write_call<5>(a_src, T::thunk);
	}

	template <class T>
	void write_thunk_call6(std::uintptr_t a_src)
	{
		SKSE::AllocTrampoline(14);

		auto& trampoline = SKSE::GetTrampoline();
		T::func = trampoline.write_call<6>(a_src, T::thunk);
	}

	template <class F, std::size_t idx, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ F::VTABLE[0] };
		T::func = vtbl.write_vfunc(idx, T::thunk);
	}

	template <std::uintptr_t address, std::size_t idx, class T>
	void write_vfunc()
	{
		REL::Relocation<std::uintptr_t> vtbl{ address };
		T::func = vtbl.write_vfunc(idx, T::thunk);
	}

	template <std::size_t idx, class T>
	void write_vfunc(std::uintptr_t id)
	{
		REL::Relocation<std::uintptr_t> vtbl{ id };
		T::func = vtbl.write_vfunc(idx, T::thunk);
	}
}

namespace logger = SKSE::log;

namespace util
{
	using SKSE::stl::report_and_fail;
}

#define DLLEXPORT __declspec(dllexport)

#include "Plugin.h"

struct MenuScreenData
{
	uint32_t unk00;         // 00
	float    mouseX;        // 04
	float    mouseY;        // 08
	uint32_t unk0C;         // 0C
	uint32_t unk10;         // 10
	float    screenWidth;   // 14
	float    screenHeight;  // 18
	uint32_t unk1C;         // 1C
	uint64_t unk20;         // 20
	uint64_t unk28;         // 28
};
static_assert(sizeof(MenuScreenData) == 0x30);
