#pragma comment(lib, "d3d11.lib")
#include <d3d11.h>
#include <dxgi.h>

#include <Detours.h>
#include "SkyrimUpscaler.h"
#include "SettingGUI.h"

#include "DRS.h"

ID3D11Device*        g_Device;
ID3D11DeviceContext* g_DeviceContext;
IDXGISwapChain*      g_SwapChain;

decltype(&IDXGISwapChain::Present)         ptrPresent;
decltype(&D3D11CreateDeviceAndSwapChain)   ptrD3D11CreateDeviceAndSwapChain;

HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	SettingGUI::GetSingleton()->OnRender();
	auto hr = (This->*ptrPresent)(SyncInterval, Flags);
	DRS::GetSingleton()->Update();
	return hr;
}

struct Hooks
{
	struct BSGraphics_Renderer_Init_InitD3D
	{
		static void thunk()
		{
			logger::info("Calling original Init3D");
			func();
			logger::info("Accessing render device information");
			auto manager = RE::BSRenderManager::GetSingleton();
			g_Device = manager->GetRuntimeData().forwarder;
			g_DeviceContext = manager->GetRuntimeData().context;
			g_SwapChain = manager->GetRuntimeData().swapChain;
			SkyrimUpscaler::GetSingleton()->SetupSwapChain(g_SwapChain);
			SettingGUI::GetSingleton()->InitIMGUI(g_SwapChain, g_Device, g_DeviceContext);
			logger::info("Detouring virtual function tables");
			*(uintptr_t*)&ptrPresent = Detours::X64::DetourClassVTable(*(uintptr_t*)g_SwapChain, &hk_IDXGISwapChain_Present, 8);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	static void Install()
	{
		stl::write_thunk_call<BSGraphics_Renderer_Init_InitD3D>(REL::RelocationID(75595, 77226).address() + REL::Relocate(0x50, 0x2BC));
		logger::info("Installed render startup hook");
	}
};

void PatchD3D11()
{
	Hooks::Install();
}
