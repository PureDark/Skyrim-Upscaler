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

decltype(&IDXGISwapChain::Present)       ptrPresent;
decltype(&ID3D11Device::CreateTexture2D) ptrCreateTexture2D;
decltype(&ID3D11DeviceContext::PSSetSamplers) ptrPSSetSamplers;
decltype(&ID3D11DeviceContext::VSSetSamplers) ptrVSSetSamplers;
decltype(&ID3D11DeviceContext::GSSetSamplers) ptrGSSetSamplers;
decltype(&ID3D11DeviceContext::HSSetSamplers) ptrHSSetSamplers;
decltype(&ID3D11DeviceContext::DSSetSamplers) ptrDSSetSamplers;
decltype(&ID3D11DeviceContext::CSSetSamplers) ptrCSSetSamplers;

HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	SettingGUI::GetSingleton()->OnRender();
	auto hr = (This->*ptrPresent)(SyncInterval, Flags);
	DRS::GetSingleton()->Update();
	return hr;
}

HRESULT WINAPI hk_ID3D11Device_CreateTexture2D(ID3D11Device* This, const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D)
{
	auto hr = (This->*ptrCreateTexture2D)(pDesc, pInitialData, ppTexture2D);
	
	return hr;
}

void WINAPI hk_ID3D11DeviceContext_PSSetSamplers(ID3D11DeviceContext* This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	(This->*ptrPSSetSamplers)(StartSlot, NumSamplers, ppSamplers);
}

void WINAPI hk_ID3D11DeviceContext_VSSetSamplers(ID3D11DeviceContext* This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	(This->*ptrVSSetSamplers)(StartSlot, NumSamplers, ppSamplers);
}

void WINAPI hk_ID3D11DeviceContext_GSSetSamplers(ID3D11DeviceContext* This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	(This->*ptrGSSetSamplers)(StartSlot, NumSamplers, ppSamplers);
}

void WINAPI hk_ID3D11DeviceContext_HSSetSamplers(ID3D11DeviceContext* This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	(This->*ptrHSSetSamplers)(StartSlot, NumSamplers, ppSamplers);
}

void WINAPI hk_ID3D11DeviceContext_DSSetSamplers(ID3D11DeviceContext* This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	(This->*ptrDSSetSamplers)(StartSlot, NumSamplers, ppSamplers);
}

void WINAPI hk_ID3D11DeviceContext_CSSetSamplers(ID3D11DeviceContext* This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	(This->*ptrCSSetSamplers)(StartSlot, NumSamplers, ppSamplers);
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
			*(uintptr_t*)&ptrCreateTexture2D = Detours::X64::DetourClassVTable(*(uintptr_t*)g_Device, &hk_ID3D11Device_CreateTexture2D, 5);
			*(uintptr_t*)&ptrPSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)g_DeviceContext, &hk_ID3D11DeviceContext_PSSetSamplers, 10);
			*(uintptr_t*)&ptrVSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)g_DeviceContext, &hk_ID3D11DeviceContext_VSSetSamplers, 26);
			*(uintptr_t*)&ptrGSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)g_DeviceContext, &hk_ID3D11DeviceContext_GSSetSamplers, 32);
			*(uintptr_t*)&ptrHSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)g_DeviceContext, &hk_ID3D11DeviceContext_HSSetSamplers, 61);
			*(uintptr_t*)&ptrDSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)g_DeviceContext, &hk_ID3D11DeviceContext_DSSetSamplers, 65);
			*(uintptr_t*)&ptrCSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)g_DeviceContext, &hk_ID3D11DeviceContext_CSSetSamplers, 70);
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
