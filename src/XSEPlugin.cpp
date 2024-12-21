
/***************SKSE PLUGIN***************/

#include "Plugin.h"
#include <DRS.h>
#include <SkyrimUpscaler.h>

static void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		if (!REL::Module::IsVR()) {
			RE::BSInputDeviceManager::GetSingleton()->AddEventSink(InputListener::GetSingleton());
			logger::info("Input listener registered");
		}
		break;
	}
	DRS::GetSingleton()->MessageHandler(a_msg);
	SkyrimUpscaler::GetSingleton()->MessageHandler(a_msg);
}

void Init()
{
	SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
	DRS::InstallHooks();
	InstallUpscalerHooks();
}

void InitializeLog()
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		util::report_and_fail("Failed to find standard logging directory"sv);
	}

	*path /= std::format("{}.log"sv, Plugin::NAME);
	auto       sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
	const auto level = spdlog::level::trace;
#else
	const auto level = spdlog::level::info;
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
	log->set_level(level);
	log->flush_on(level);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%l] %v"s);
}

std::string GetLibraryPath(std::string filename)
{
	char    path[MAX_PATH];
	HMODULE hm = NULL;

	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
							  GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCSTR)&Init, &hm) == 0) {
		int ret = GetLastError();
		fprintf(stderr, "GetModuleHandle failed, error = %d\n", ret);
		// Return or however you want to handle an error.
	}
	if (GetModuleFileNameA(hm, path, sizeof(path)) == 0) {
		int ret = GetLastError();
		fprintf(stderr, "GetModuleFileName failed, error = %d\n", ret);
		// Return or however you want to handle an error.
	}

	std::string::size_type pos = std::string(path).find_last_of("\\/");
	return (std::string(path).substr(0, pos) + "/../../UpscalerBasePlugin/" + filename);
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {};
#endif

	InitializeLog();
	auto handle = LoadLibrary(GetLibraryPath("nvngx_dlss.dll"s).c_str());
	logger::info("Loaded plugin nvngx_dlss.dll {}", (void*)handle);

#ifndef NDEBUG
	handle = LoadLibrary(GetLibraryPath("ffx_backend_dx11_x64d.dll").c_str());
	spdlog::info("Loaded plugin ffx_backend_dx11_x64d.dll {}", (void*)handle);
	handle = LoadLibrary(GetLibraryPath("ffx_fsr3upscaler_x64d.dll").c_str());
	spdlog::info("Loaded plugin ffx_fsr3upscaler_x64d.dll {}", (void*)handle);
	handle = LoadLibrary(GetLibraryPath("ffx_fsr3_x64d.dll").c_str());
	spdlog::info("Loaded plugin ffx_fsr3_x64d.dll {}", (void*)handle);
#else
	handle = LoadLibrary(GetLibraryPath("ffx_backend_dx11_x64.dll").c_str());
	spdlog::info("Loaded plugin ffx_backend_dx11_x64.dll {}", (void*)handle);
	handle = LoadLibrary(GetLibraryPath("ffx_fsr3upscaler_x64.dll").c_str());
	spdlog::info("Loaded plugin ffx_fsr3upscaler_x64.dll {}", (void*)handle);
	handle = LoadLibrary(GetLibraryPath("ffx_fsr3_x64.dll").c_str());
	spdlog::info("Loaded plugin ffx_fsr3_x64.dll {}", (void*)handle);
#endif
	handle = LoadLibrary(GetLibraryPath("libxess.dll").c_str());
	logger::info("Loaded plugin libxess.dll {}", (void*)handle);
	handle = LoadLibrary(GetLibraryPath("PDPerfPlugin.dll").c_str());
	logger::info("Loaded plugin PDPerfPlugin.dll {}", (void*)handle);

	logger::info("Loaded all plugins");

	static INT64 startTime = MillisecondsNow();
	while (MillisecondsNow() - startTime <= 3 * 1000) {
	};

#ifdef _RENDERDOC
	LoadLibrary("renderdoc.dll");
#endif

	SKSE::Init(a_skse);

	Init();

	return true;
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData v;
	v.PluginName(Plugin::NAME.data());
	v.PluginVersion(Plugin::VERSION);
	v.UsesAddressLibrary();
	v.UsesNoStructs();
	return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}

static void on_init_effect_runtime(effect_runtime* runtime)
{
	uint32_t width, height;
	runtime->get_screenshot_width_and_height(&width, &height);
	if (width > 1024 && height > 1024) {
		SkyrimUpscaler::GetSingleton()->m_runtime = runtime;
	}
}

void register_addon_events()
{
	reshade::register_event<reshade::addon_event::init_effect_runtime>(on_init_effect_runtime);
}

void unregister_addon_events()
{
	reshade::unregister_event<reshade::addon_event::init_effect_runtime>(on_init_effect_runtime);
}

//extern "C" __declspec(dllexport) const char* NAME = "Skyrim Upscaler";
//extern "C" __declspec(dllexport) const char* DESCRIPTION = "Apply effects to correct rendertargets in VR";
//static bool ReShadeInstalled = false;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
#ifdef _RENDERDOC

#endif
		//ReShadeInstalled = true;
		//if (!reshade::register_addon(hModule)) {
		//	ReShadeInstalled = false;
		//	return TRUE;
		//}
		//register_addon_events();
		break;
	case DLL_PROCESS_DETACH:
		//if (ReShadeInstalled) {
		//	unregister_addon_events();
		//	reshade::unregister_addon(hModule);
		//}
		break;
	}

	return TRUE;
}
