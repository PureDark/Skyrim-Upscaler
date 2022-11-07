
/***************SKSE PLUGIN***************/

#include "Plugin.h"
#include <ENB/ENBSeriesAPI.h>
#include <DRS.h>
#include <SkyrimUpscaler.h>

ENB_API::ENBSDKALT1001* g_ENB = nullptr;

static void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		RE::BSInputDeviceManager::GetSingleton()->AddEventSink(InputListener::GetSingleton());
		logger::info("Input listener registered");
		break;
	case SKSE::MessagingInterface::kPostLoad:
		g_ENB = reinterpret_cast<ENB_API::ENBSDKALT1001*>(ENB_API::RequestENBAPI(ENB_API::SDKVersion::V1001));
		if (g_ENB) {
			logger::info("Obtained ENB API");
		} else
			logger::info("Unable to acquire ENB API");
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
	MenuOpenCloseEventHandler::Register();
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

std::string MyGetDllDirectory()
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
	return std::string(path).substr(0, pos);
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {};
#endif

	InitializeLog();
	logger::info("DLL Path : {}", MyGetDllDirectory());
	auto filename = MyGetDllDirectory() + "/SkyrimUpscaler/nvngx_dlss.dll";
	LoadLibrary(filename.c_str());
	filename = MyGetDllDirectory() + "/SkyrimUpscaler/ffx_fsr2_api_x64.dll";
	LoadLibrary(filename.c_str());
	filename = MyGetDllDirectory() + "/SkyrimUpscaler/ffx_fsr2_api_dx12_x64.dll";
	LoadLibrary(filename.c_str());
	filename = MyGetDllDirectory() + "/SkyrimUpscaler/dxil.dll";
	LoadLibrary(filename.c_str());
	filename = MyGetDllDirectory() + "/SkyrimUpscaler/dxcompiler.dll";
	LoadLibrary(filename.c_str());
	filename = MyGetDllDirectory() + "/SkyrimUpscaler/XeFX.dll";
	LoadLibrary(filename.c_str());
	filename = MyGetDllDirectory() + "/SkyrimUpscaler/XeFX_Loader.dll";
	LoadLibrary(filename.c_str());
	filename = MyGetDllDirectory() + "/SkyrimUpscaler/libxess.dll";
	LoadLibrary(filename.c_str());
	filename = MyGetDllDirectory() + "/SkyrimUpscaler/PDPerfPlugin.dll";
	LoadLibrary(filename.c_str());
	logger::info("Loaded plugin");

	SKSE::Init(a_skse);

	Init();

	return true;
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData v;
	v.PluginName(Plugin::NAME);
	v.PluginVersion(Plugin::VERSION);
	v.UsesAddressLibrary(true);
	v.HasNoStructUse(true);
	return v;
}();

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
	pluginInfo->name = SKSEPlugin_Version.pluginName;
	pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
	pluginInfo->version = SKSEPlugin_Version.pluginVersion;
	return true;
}
