#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <Detours.h>
#include "SkyrimUpscaler.h"
#include "SettingGUI.h"
#include "d3d11_proxy.h"
#include "imgui.h"
#include <DRS.h>

static DXGISwapChainProxy* SwapChainProxy;

static float                                                        mipLodBias = 0;
static std::unordered_set<ID3D11SamplerState*>                      passThroughSamplers;
static std::unordered_map<ID3D11SamplerState*, ID3D11SamplerState*> mappedSamplers;

decltype(&IDXGIFactory::CreateSwapChain)      ptrCreateSwapChain;
decltype(&D3D11CreateDeviceAndSwapChain)      ptrD3D11CreateDeviceAndSwapChain;
decltype(&IDXGISwapChain::Present)            ptrPresent;
decltype(&IDXGISwapChain::GetFullscreenState) ptrGetFullscreenState;
decltype(&ID3D11Device::CreateTexture2D)      ptrCreateTexture2D;
decltype(&ID3D11DeviceContext::PSSetSamplers) ptrPSSetSamplers;
decltype(&ID3D11DeviceContext::VSSetSamplers) ptrVSSetSamplers;
decltype(&ID3D11DeviceContext::GSSetSamplers) ptrGSSetSamplers;
decltype(&ID3D11DeviceContext::HSSetSamplers) ptrHSSetSamplers;
decltype(&ID3D11DeviceContext::DSSetSamplers) ptrDSSetSamplers;
decltype(&ID3D11DeviceContext::CSSetSamplers) ptrCSSetSamplers;

decltype(&ID3D11DeviceContext::OMSetRenderTargets) ptrOMSetRenderTargets;
decltype(&ID3D11DeviceContext::RSSetViewports)     ptrRSSetViewports;
static bool                                        bSwitchRTV = false;
static ID3D11Texture2D*                            backbuffer;
static ID3D11Texture2D*                            backbuffer2;
static ID3D11RenderTargetView*                     backbufferRTV;

static void MyLog(char* message, int size)
{
	logger::info("{}", message);
}

void WINAPI hk_ID3D11DeviceContext_OMSetRenderTargets(ID3D11DeviceContext* This, UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView)
{
	if (backbufferRTV != nullptr && bSwitchRTV && NumViews > 0) {
		ID3D11RenderTargetView* rtvs[2] = { backbufferRTV, nullptr };
		ID3D11Resource* resource;
		ppRenderTargetViews[0]->GetResource(&resource);
		if (resource == backbuffer2) {
			if (NumViews > 1)
				rtvs[1] = SkyrimUpscaler::GetSingleton()->mMotionVectorsEmpty.GetRTV();
			auto dsv = (pDepthStencilView != nullptr) ? SkyrimUpscaler::GetSingleton()->mTempDepthBuffer.GetDSV() : nullptr;
			(This->*ptrOMSetRenderTargets)(NumViews, rtvs, dsv);
			return;
		}
	}
	(This->*ptrOMSetRenderTargets)(NumViews, ppRenderTargetViews, pDepthStencilView);
}

void WINAPI hk_ID3D11DeviceContext_RSSetViewports(ID3D11DeviceContext* This, UINT NumViewports, const D3D11_VIEWPORT* pViewports)
{
	if (backbufferRTV != nullptr && bSwitchRTV && NumViewports == 1 && 
		pViewports[0].Width == SkyrimUpscaler::GetSingleton()->mRenderSizeX &&
		pViewports[0].Height == SkyrimUpscaler::GetSingleton()->mRenderSizeY) {
		D3D11_VIEWPORT viewports;
		viewports.Width = SkyrimUpscaler::GetSingleton()->mDisplaySizeX;
		viewports.Height = SkyrimUpscaler::GetSingleton()->mDisplaySizeY;
		viewports.TopLeftX = pViewports[0].TopLeftX;
		viewports.TopLeftY = pViewports[0].TopLeftY;
		viewports.MaxDepth = pViewports[0].MaxDepth;
		viewports.MinDepth = pViewports[0].MinDepth;
		(This->*ptrRSSetViewports)(NumViewports, &viewports);
	} else {
		(This->*ptrRSSetViewports)(NumViewports, pViewports);
	}
}



HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	bSwitchRTV = false;
	// Prevents swapchain2 from actually doing present, saving performance
	// It's needed for ENB to be informed, but not needed to do actual present
	if (SwapChainProxy->blockPresent)
		return 0;
	auto hr = (This->*ptrPresent)(SyncInterval, Flags);
	return hr;
}

HRESULT WINAPI hk_IDXGISwapChain_GetFullscreenState(IDXGISwapChain* This, BOOL* pFullscreen, IDXGIOutput** ppTarget)
{
	bSwitchRTV = DRS::GetSingleton()->IsInFullscreenMenu();
	auto hr = (This->*ptrGetFullscreenState)(pFullscreen, ppTarget);
	return hr;
}

HRESULT WINAPI hk_ID3D11Device_CreateTexture2D(ID3D11Device* This, const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D)
{
	auto hr = (This->*ptrCreateTexture2D)(pDesc, pInitialData, ppTexture2D);
	if (pDesc->Format >= DXGI_FORMAT_R16G16_TYPELESS && pDesc->Format <= DXGI_FORMAT_R16G16_SINT) {
		bool exist = false;
		for (const motion_item& item : SettingGUI::GetSingleton()->sorted_item_list) {
			if (item.resource == *ppTexture2D)
				exist = true;
		}
		if (!exist) {
			motion_item item = { 1u, *ppTexture2D, *pDesc };
			if (SettingGUI::GetSingleton()->sorted_item_list.size() == 0) {
				SettingGUI::GetSingleton()->sorted_item_list.push_back(item);
				SettingGUI::GetSingleton()->selected_item = item;
				SkyrimUpscaler::GetSingleton()->SetupMotionVector(SettingGUI::GetSingleton()->selected_item.resource);
				SkyrimUpscaler::GetSingleton()->InitUpscaler();
				logger::info("Motion Vertor Found : {} x {}", pDesc->Width, pDesc->Height);
			}
		}
	} else if (pDesc->Format >= DXGI_FORMAT_R24G8_TYPELESS && pDesc->Format <= DXGI_FORMAT_X24_TYPELESS_G8_UINT) {
		if (pDesc->Width == SkyrimUpscaler::GetSingleton()->mRenderSizeX &&
			pDesc->Height == SkyrimUpscaler::GetSingleton()->mRenderSizeY &&
			SkyrimUpscaler::GetSingleton()->mDepthBuffer.mImage == nullptr) {
			SkyrimUpscaler::GetSingleton()->SetupDepth(*ppTexture2D);
			logger::info("Depth Buffer Found : {} x {}", pDesc->Width, pDesc->Height);
		}
	}
	return hr;
}

// Mostly from vrperfkit, thanks to fholger for showing how to do mip lod bias
// https://github.com/fholger/vrperfkit/blob/037c09f3168ac045b5775e8d1a0c8ac982b5854f/src/d3d11/d3d11_post_processor.cpp#L76
static void SetMipLodBias(ID3D11SamplerState** outSamplers, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	if (mipLodBias != SkyrimUpscaler::GetSingleton()->mMipLodBias) {
		logger::info("MIP LOD Bias changed from  {} to {}, recreating samplers", mipLodBias, SkyrimUpscaler::GetSingleton()->mMipLodBias);
		passThroughSamplers.clear();
		mappedSamplers.clear();
		mipLodBias = SkyrimUpscaler::GetSingleton()->mMipLodBias;
	}
	memcpy(outSamplers, ppSamplers, NumSamplers * sizeof(ID3D11SamplerState*));
	for (UINT i = 0; i < NumSamplers; ++i) {
		auto orig = outSamplers[i];
		if (orig == nullptr || passThroughSamplers.find(orig) != passThroughSamplers.end()) {
			continue;
		}
		if (mappedSamplers.find(orig) == mappedSamplers.end()) {
			D3D11_SAMPLER_DESC sd;
			orig->GetDesc(&sd);
			if (sd.MipLODBias != 0 || sd.MaxAnisotropy <= 1) {
				// do not mess with samplers that already have a bias or are not doing anisotropic filtering.
				// should hopefully reduce the chance of causing rendering errors.
				passThroughSamplers.insert(orig);
				continue;
			}
			if (DRS::GetSingleton()->IsInFullscreenMenu()) 
				continue;
			sd.MipLODBias = mipLodBias;
			SkyrimUpscaler::GetSingleton()->mDevice->CreateSamplerState(&sd, &mappedSamplers[orig]);
			passThroughSamplers.insert(mappedSamplers[orig]);
		}
		outSamplers[i] = mappedSamplers[orig];
	}
}

void WINAPI hk_ID3D11DeviceContext_PSSetSamplers(ID3D11DeviceContext* This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	ID3D11SamplerState* samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SetMipLodBias(samplers, StartSlot, NumSamplers, ppSamplers);
	(This->*ptrPSSetSamplers)(StartSlot, NumSamplers, samplers);
}

void WINAPI hk_ID3D11DeviceContext_VSSetSamplers(ID3D11DeviceContext* This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	ID3D11SamplerState* samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SetMipLodBias(samplers, StartSlot, NumSamplers, ppSamplers);
	(This->*ptrVSSetSamplers)(StartSlot, NumSamplers, samplers);
}

void WINAPI hk_ID3D11DeviceContext_GSSetSamplers(ID3D11DeviceContext* This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	ID3D11SamplerState* samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SetMipLodBias(samplers, StartSlot, NumSamplers, ppSamplers);
	(This->*ptrGSSetSamplers)(StartSlot, NumSamplers, samplers);
}

void WINAPI hk_ID3D11DeviceContext_HSSetSamplers(ID3D11DeviceContext* This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	ID3D11SamplerState* samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SetMipLodBias(samplers, StartSlot, NumSamplers, ppSamplers);
	(This->*ptrHSSetSamplers)(StartSlot, NumSamplers, samplers);
}

void WINAPI hk_ID3D11DeviceContext_DSSetSamplers(ID3D11DeviceContext* This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	ID3D11SamplerState* samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SetMipLodBias(samplers, StartSlot, NumSamplers, ppSamplers);
	(This->*ptrDSSetSamplers)(StartSlot, NumSamplers, samplers);
}

void WINAPI hk_ID3D11DeviceContext_CSSetSamplers(ID3D11DeviceContext* This, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	ID3D11SamplerState* samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SetMipLodBias(samplers, StartSlot, NumSamplers, ppSamplers);
	(This->*ptrCSSetSamplers)(StartSlot, NumSamplers, samplers);
}

HRESULT WINAPI hk_IDXGIFactory_CreateSwapChain(IDXGIFactory2* This, _In_ IUnknown* pDevice, _In_ DXGI_SWAP_CHAIN_DESC* pDesc, _COM_Outptr_ IDXGISwapChain** ppSwapChain)
{
	auto hr = (This->*ptrCreateSwapChain)(pDevice, pDesc, ppSwapChain);
	logger::info("hk_IDXGIFactory_CreateSwapChain created original SwapChain : {} x {} Format: {}", pDesc->BufferDesc.Width, pDesc->BufferDesc.Height, pDesc->BufferDesc.Format);
	static bool init = false;
	if (!init) {
		init = true;
		SwapChainProxy = new DXGISwapChainProxy(*ppSwapChain);

		InitLogDelegate(MyLog);
		SkyrimUpscaler::GetSingleton()->LoadINI();
		SkyrimUpscaler::GetSingleton()->SetupSwapChain(*ppSwapChain);
		SkyrimUpscaler::GetSingleton()->PreInit();
		SkyrimUpscaler::GetSingleton()->InitUpscaler();

		IDXGISwapChain1* mSwapChain = NULL;

		DXGI_SWAP_CHAIN_DESC  swapChainDesc = *pDesc;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc1 = {};
		swapChainDesc1.Width = SkyrimUpscaler::GetSingleton()->mRenderSizeX;
		swapChainDesc1.Height = SkyrimUpscaler::GetSingleton()->mRenderSizeY;
		swapChainDesc1.Format = swapChainDesc.BufferDesc.Format;
		swapChainDesc1.Stereo = false;
		swapChainDesc1.SampleDesc = swapChainDesc.SampleDesc;
		swapChainDesc1.BufferUsage = swapChainDesc.BufferUsage;
		swapChainDesc1.BufferCount = swapChainDesc.BufferCount;
		swapChainDesc1.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		swapChainDesc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc1.Flags = swapChainDesc.Flags;

		HRESULT hResult = This->CreateSwapChainForComposition(pDevice, &swapChainDesc1, nullptr, &mSwapChain);
		mSwapChain->QueryInterface(IID_PPV_ARGS(&SwapChainProxy->mSwapChain2));

		*ppSwapChain = SwapChainProxy->mSwapChain2;
	}
	return hr;
}

HRESULT WINAPI hk_D3D11CreateDeviceAndSwapChain(
	IDXGIAdapter*               pAdapter,
	D3D_DRIVER_TYPE             DriverType,
	HMODULE                     Software,
	UINT                        Flags,
	const D3D_FEATURE_LEVEL*    pFeatureLevels,
	UINT                        FeatureLevels,
	UINT                        SDKVersion,
	const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
	IDXGISwapChain**            ppSwapChain,
	ID3D11Device**              ppDevice,
	D3D_FEATURE_LEVEL*          pFeatureLevel,
	ID3D11DeviceContext**       ppImmediateContext)
{
	static bool init = false;
	if (!init) {
		IDXGIFactory2* mFactory = NULL;
		pAdapter->GetParent(IID_PPV_ARGS(&mFactory));
		*(uintptr_t*)&ptrCreateSwapChain = Detours::X64::DetourClassVTable(*(uintptr_t*)mFactory, &hk_IDXGIFactory_CreateSwapChain, 10);
	}

	logger::info("Calling original D3D11CreateDeviceAndSwapChain");
	HRESULT hr = (*ptrD3D11CreateDeviceAndSwapChain)(pAdapter,
		DriverType,
		Software,
		Flags,
		pFeatureLevels,
		FeatureLevels,
		SDKVersion,
		pSwapChainDesc,
		ppSwapChain,
		ppDevice,
		pFeatureLevel,
		ppImmediateContext);

	if (!init) {
		auto device = *ppDevice;
		auto deviceContext = *ppImmediateContext;
		auto swapChain = *ppSwapChain;
		SettingGUI::GetSingleton()->InitIMGUI(SwapChainProxy->mSwapChain1, device, deviceContext);

		logger::info("Detouring virtual function tables");
		*(uintptr_t*)&ptrPresent = Detours::X64::DetourClassVTable(*(uintptr_t*)SwapChainProxy->mSwapChain2, &hk_IDXGISwapChain_Present, 8);
		*(uintptr_t*)&ptrCreateTexture2D = Detours::X64::DetourClassVTable(*(uintptr_t*)device, &hk_ID3D11Device_CreateTexture2D, 5);
		*(uintptr_t*)&ptrPSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_PSSetSamplers, 10);
		*(uintptr_t*)&ptrVSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_VSSetSamplers, 26);
		*(uintptr_t*)&ptrGSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_GSSetSamplers, 32);
		*(uintptr_t*)&ptrHSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_HSSetSamplers, 61);
		*(uintptr_t*)&ptrDSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_DSSetSamplers, 65);
		*(uintptr_t*)&ptrCSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_CSSetSamplers, 70);

		*(uintptr_t*)&ptrOMSetRenderTargets = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_OMSetRenderTargets, 33);
		*(uintptr_t*)&ptrRSSetViewports = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_RSSetViewports, 44);
		//*(uintptr_t*)&ptrGetFullscreenState = Detours::X64::DetourClassVTable(*(uintptr_t*)SwapChainProxy->mSwapChain2, &hk_IDXGISwapChain_GetFullscreenState, 11);

		// Replace the reference of the original swapchain with the ENB SwapChain wrapper
		SwapChainProxy->mSwapChain2 = swapChain;
		// In order to make IED compatible, use the real SwapChain initially
		SwapChainProxy->usingSwapChain2 = false;
		*ppSwapChain = SwapChainProxy;
		init = true;
	}

	return hr;
}

struct UpscalerHooks
{
	struct BSGraphics_Renderer_Init_CreateD3D
	{
		static void thunk()
		{
			// To make IED compatible
			func(); // This is IED's hook
			// Switching back to the fake swapchain after IED has done with SwapChain
			SwapChainProxy->usingSwapChain2 = true;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct BSGraphics_Renderer_Init_InitD3D
	{
		static void thunk()
		{
			func();
			MenuOpenCloseEventHandler::Register();
			// Setting menu screen size after all render targets are created
			static uint32_t* g_width = (uint32_t*)REL::RelocationID(525002, 411483).address();   // 302C8B4, 30C6DB4
			static uint32_t* g_height = (uint32_t*)REL::RelocationID(525003, 411484).address();  // 302C8B8, 30C6DB8
			static uint32_t* g_xRight = (uint32_t*)REL::RelocationID(525004, 411485).address();   // 302C8BC, 30C6DBC
			static uint32_t* g_yBottom = (uint32_t*)REL::RelocationID(525005, 411486).address();  // 302C8C0, 30C6DC0
			*g_width = SkyrimUpscaler::GetSingleton()->mDisplaySizeX;
			*g_height = SkyrimUpscaler::GetSingleton()->mDisplaySizeY;
			*g_xRight = *g_width;
			*g_yBottom = *g_height;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct Main_DrawWorld_MainDraw
	{
		static void thunk(INT64 BSGraphics_Renderer, int unk)
		{
			static bool initTAA = false;
			if (!initTAA) {
				initTAA = true;
				UnkOuterStruct::GetSingleton()->SetTAA(SkyrimUpscaler::GetSingleton()->mUpscaleType == TAA);
			}
			func(BSGraphics_Renderer, unk);
			if (!backbuffer)
				SwapChainProxy->mSwapChain1->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
			if (!backbuffer2)
				SwapChainProxy->mSwapChain2->GetBuffer(0, IID_PPV_ARGS(&backbuffer2));
			if (!DRS::GetSingleton()->IsInFullscreenMenu())
				SkyrimUpscaler::GetSingleton()->ForceEvaluateUpscaler(backbuffer2, backbuffer);

			if (backbufferRTV == nullptr)
				SwapChainProxy->mDevice->CreateRenderTargetView(backbuffer, NULL, &backbufferRTV);

			auto motionRTV = SkyrimUpscaler::GetSingleton()->mMotionVectorsEmpty.GetRTV();
			const FLOAT color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			SwapChainProxy->mContext->ClearRenderTargetView(motionRTV, color);

			// Something is odd with main menu, should be dealt with separately
			if (!DRS::GetSingleton()->IsInFullscreenMenu()) {
				auto DSV2 = SkyrimUpscaler::GetSingleton()->mTempDepthBuffer.GetDSV();
				SwapChainProxy->mContext->ClearDepthStencilView(DSV2, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
				SwapChainProxy->mContext->OMSetRenderTargets(1, &backbufferRTV, DSV2);
				bSwitchRTV = true;
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct BSGraphics_Renderer_Begin_UpdateJitter
	{
		static void thunk(BSGraphics::State* a_state)
		{
			func(a_state);
			if (SkyrimUpscaler::GetSingleton()->IsEnabled() && 
				SkyrimUpscaler::GetSingleton()->mEnableJitter &&
				!DRS::GetSingleton()->IsInFullscreenMenu()) {
				if (SkyrimUpscaler::GetSingleton()->mUpscaleType != TAA) {
					float x = 0.0f;
					float y = 0.0f;
					SkyrimUpscaler::GetSingleton()->GetJitters(&x, &y);
					float w = SkyrimUpscaler::GetSingleton()->mRenderSizeX;
					float h = SkyrimUpscaler::GetSingleton()->mRenderSizeY;
					a_state->jitter[0] = -2 * x / w;
					a_state->jitter[1] = 2 * y / h;
					SkyrimUpscaler::GetSingleton()->SetJitterOffsets(-x, -y);
				}
			} else {
				a_state->jitter[0] = 0;
				a_state->jitter[1] = 0;
				SkyrimUpscaler::GetSingleton()->SetJitterOffsets(0, 0);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct GetClientRectHook
	{
		static BOOL thunk(_In_ HWND hWnd, _Out_ LPRECT lpRect)
		{
			BOOL result = ::GetClientRect(hWnd, lpRect);
			lpRect->right = SkyrimUpscaler::GetSingleton()->mRenderSizeX;
			lpRect->bottom = SkyrimUpscaler::GetSingleton()->mRenderSizeY;
			//logger::info("lpRect size : {}x{}", lpRect->right, lpRect->bottom);
			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct MistMenu_PostDisplay
	{
		static void thunk(void* a1, uint32_t a2, uint32_t a3)
		{
			func(a1, a2, a3);

			SkyrimUpscaler::GetSingleton()->ForceEvaluateUpscaler(backbuffer2, backbuffer);

			auto DSV2 = SkyrimUpscaler::GetSingleton()->mTempDepthBuffer.GetDSV();
			SwapChainProxy->mContext->OMSetRenderTargets(1, &backbufferRTV, DSV2);
			bSwitchRTV = true;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct MenuEventHandler_ProcessMouseMove
	{
		static void thunk(MenuScreenData* a_menuScreenData, RE::MouseMoveEvent* a_event)
		{
			a_menuScreenData->screenWidth = SkyrimUpscaler::GetSingleton()->mDisplaySizeX;
			a_menuScreenData->screenHeight = SkyrimUpscaler::GetSingleton()->mDisplaySizeY;
			func(a_menuScreenData, a_event);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct ScreenRect
	{
		uint32_t width;
		uint32_t height;
	};

	struct SetScreenSize
	{
		static void thunk(void* a1, ScreenRect& a2)
		{
			a2.width = SkyrimUpscaler::GetSingleton()->mDisplaySizeX;
			a2.height = SkyrimUpscaler::GetSingleton()->mDisplaySizeY;
			func(a1, a2);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	static void Install()
	{
		// Hook for getting the swapchain
		stl::write_thunk_call<BSGraphics_Renderer_Init_CreateD3D>(REL::RelocationID(75595, 77226).address() + REL::Relocate(0x9, 0x275));
		stl::write_thunk_call<BSGraphics_Renderer_Init_InitD3D>(REL::RelocationID(75595, 77226).address() + REL::Relocate(0x50, 0x2BC));
		// Have to hook the creation of SwapChain to hook CreateTexture2D before depth and motion textures are created
		char* ptr = nullptr;
		auto  moduleBase = (uintptr_t)GetModuleHandle(ptr);
		auto  dllD3D11 = GetModuleHandleA("d3d11.dll");
		*(FARPROC*)&ptrD3D11CreateDeviceAndSwapChain = GetProcAddress(dllD3D11, "D3D11CreateDeviceAndSwapChain");
		Detours::IATHook(moduleBase, "d3d11.dll", "D3D11CreateDeviceAndSwapChain", (uintptr_t)hk_D3D11CreateDeviceAndSwapChain);
		 
		// Pre-UI Hook for upscaling
		stl::write_thunk_call<Main_DrawWorld_MainDraw>(REL::RelocationID(79947, 82084).address() + REL::Relocate(0x16F, 0x17A));  // EBF510 (EBF67F), F05BF0 (F05D6A)
		// Setup our own jitters
		stl::write_thunk_call<BSGraphics_Renderer_Begin_UpdateJitter>(REL::RelocationID(75460, 77245).address() + REL::Relocate(0xE5, 0xE2));  // D6A0C0 (D6A1A5), DA5A00 (DA5AE2)
		// GetClientRectHook
		stl::write_thunk_call6<GetClientRectHook>(REL::RelocationID(75460, 77245).address() + REL::Relocate(0x192, 0x18B));  // D6A0C0 (D6A252), DA5A00 (DA5B8B)

		stl::write_thunk_call<MistMenu_PostDisplay>(REL::RelocationID(51855, 52727).address() + REL::Relocate(0x7A1, 0x7A4));      // 8D2390 (8D2B31), 9017E0 (901F84)
		
		// unlock cursor from low resolution rect
		stl::write_thunk_call<MenuEventHandler_ProcessMouseMove>(REL::RelocationID(50604, 51498).address() + REL::Relocate(0xB, 0xB));  // 875A40 (875A4B), 8A3FD0 (8A3FDB)
		stl::write_thunk_call<SetScreenSize>(REL::RelocationID(75590, 77397).address() + REL::Relocate(0x102, 0x102));                  // D71D20 (D71E22), DADAF0 (DADBF2)

		// Always enable TAA jitters, even without TAA
		static REL::Relocation<uintptr_t> updateJitterHook{ REL::RelocationID(75709, 77518) };          // D7CFB0, DB96E0
		static REL::Relocation<uintptr_t> buildCameraStateDataHook{ REL::RelocationID(75711, 77520) };  // D7D130, DB9850
		uint8_t                           patch1[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
		uint8_t                           patch2[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
		REL::safe_write<uint8_t>(updateJitterHook.address() + REL::Relocate(0xE, 0x11), patch1);
		REL::safe_write<uint8_t>(buildCameraStateDataHook.address() + REL::Relocate(0x1D5, 0x1D5), patch2);
		logger::info("Installed upscaler hooks");
	}
};

void InstallUpscalerHooks() {
	UpscalerHooks::Install();
}

