
/***************SKSE PLUGIN***************/

#include "Plugin.h"
#include <ENB/ENBSeriesAPI.h>
#include <DRS.h>
#include <SkyrimUpscaler.h>

static void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		RE::BSInputDeviceManager::GetSingleton()->AddEventSink(InputListener::GetSingleton());
		logger::info("Input listener registered");
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
	return (std::string(path).substr(0, pos) + "/SkyrimUpscaler/" + filename);
}

int PrintModules(DWORD processID)
{
	HMODULE      hMods[1024];
	HANDLE       hProcess;
	DWORD        cbNeeded;
	unsigned int i;

	// Print the process identifier.

	printf("\nProcess ID: %u\n", processID);
	logger::info("\nProcess ID: {}\n", processID);

	// Get a handle to the process.

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
							   PROCESS_VM_READ,
		FALSE, processID);
	if (NULL == hProcess)
		return 1;

	// Get a list of all the modules in this process.

	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
		for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
			TCHAR szModName[MAX_PATH];

			// Get the full path to the module's file.

			if (GetModuleFileNameEx(hProcess, hMods[i], szModName,
					sizeof(szModName) / sizeof(TCHAR))) {
				// Print the module name and handle value.

				_tprintf(TEXT("\t%s (0x%08X)\n"), szModName, hMods[i]);
				char str[512];
				sprintf_s(str, "\t%s (0x%08X)\n", szModName, hMods[i]);
				logger::info("{}", str);
			}
		}
	}

	// Release the handle to the process.

	CloseHandle(hProcess);

	return 0;
}

EXTERN_C [[maybe_unused]] __declspec(dllexport) bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {};
#endif

	InitializeLog();
	LoadLibrary(GetLibraryPath("nvngx_dlss.dll").c_str());
	LoadLibrary(GetLibraryPath("ffx_fsr2_api_x64.dll").c_str());
	LoadLibrary(GetLibraryPath("ffx_fsr2_api_dx12_x64.dll").c_str());
	LoadLibrary(GetLibraryPath("dxil.dll").c_str());
	LoadLibrary(GetLibraryPath("dxcompiler.dll").c_str());
	LoadLibrary(GetLibraryPath("XeFX.dll").c_str());
	LoadLibrary(GetLibraryPath("XeFX_Loader.dll").c_str());
	LoadLibrary(GetLibraryPath("libxess.dll").c_str());
	LoadLibrary(GetLibraryPath("PDPerfPlugin.dll").c_str());
	logger::info("Loaded plugin");

	SKSE::Init(a_skse);

	Init();

	//DWORD        aProcesses[1024];
	//DWORD        cbNeeded;
	//DWORD        cProcesses;
	//unsigned int i;

	//// Get the list of process identifiers.

	//if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	//	return 1;

	//// Calculate how many process identifiers were returned.

	//cProcesses = cbNeeded / sizeof(DWORD);

	//// Print the names of the modules for each process.

	//for (i = 0; i < cProcesses; i++) {
	//	PrintModules(aProcesses[i]);
	//}
	PrintModules(GetCurrentProcessId());


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
