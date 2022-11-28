#pragma once
#include <d3d11.h>
#include <dxgi.h>

#include <Detours.h>
#include "SkyrimUpscaler.h"
#include "SettingGUI.h"

#include <Detours.h>
#include "DRS.h"
#include <ScreenGrab11.h>
#include <wincodec.h>
#include <openvr.h>

static float                                                        mipLodBias = 0;
static std::unordered_set<ID3D11SamplerState*>                      passThroughSamplers;
static std::unordered_map<ID3D11SamplerState*, ID3D11SamplerState*> mappedSamplers;
static std::map<int, ID3D11Texture2D*>                              targetMap;
static int                                                          size;
static std::map<int, ID3D11Texture2D*>                              targetMap2;
static int                                                          size2;
static int                                                          index;

decltype(&D3D11CreateDeviceAndSwapChain)      ptrD3D11CreateDeviceAndSwapChain;
decltype(&IDXGISwapChain::Present)            ptrPresent;
decltype(&ID3D11Device::CreateTexture2D)      ptrCreateTexture2D;
decltype(&ID3D11DeviceContext::PSSetSamplers) ptrPSSetSamplers;
decltype(&ID3D11DeviceContext::VSSetSamplers) ptrVSSetSamplers;
decltype(&ID3D11DeviceContext::GSSetSamplers) ptrGSSetSamplers;
decltype(&ID3D11DeviceContext::HSSetSamplers) ptrHSSetSamplers;
decltype(&ID3D11DeviceContext::DSSetSamplers) ptrDSSetSamplers;
decltype(&ID3D11DeviceContext::CSSetSamplers) ptrCSSetSamplers;
decltype(&vr::IVRCompositor::Submit)          ptrSubmit;

decltype(&ID3D11DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews) ptrOMSetRenderTargetsAndUnorderedAccessViews;

static bool flagSkipDrawCall = false;

static void MyLog(char* message, int size)
{
	logger::info("{}", message);
}

void WINAPI hk_ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews(ID3D11DeviceContext* This, UINT NumRTVs, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView,
	UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView* const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts)
{
	if (flagSkipDrawCall) {
		flagSkipDrawCall = false;
		(This->*ptrOMSetRenderTargetsAndUnorderedAccessViews)(0, nullptr, nullptr, UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
		ID3D11Resource* dest;
		(*ppRenderTargetViews)->GetResource(&dest);
		SkyrimUpscaler::GetSingleton()->mContext->CopyResource(dest, SkyrimUpscaler::GetSingleton()->mTargetTex.mImage);
		return;
	}
	(This->*ptrOMSetRenderTargetsAndUnorderedAccessViews)(NumRTVs, ppRenderTargetViews, pDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
}

HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	SettingGUI::GetSingleton()->OnRender();
	auto hr = (This->*ptrPresent)(SyncInterval, Flags);
	DRS::GetSingleton()->Update();
	return 0;
}

HRESULT WINAPI hk_ID3D11Device_CreateTexture2D(ID3D11Device* This, const D3D11_TEXTURE2D_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D** ppTexture2D)
{
	auto hr = (This->*ptrCreateTexture2D)(pDesc, pInitialData, ppTexture2D);
	static bool creating = false;
	if (creating)
		return hr;
	if (pDesc->Format == DXGI_FORMAT_R16G16_FLOAT && pDesc->BindFlags == (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET)) {
		if (SkyrimUpscaler::GetSingleton()->mDisplaySizeX > 0) {
			if (pDesc->Width != SkyrimUpscaler::GetSingleton()->mDisplaySizeX ||
				pDesc->Height != SkyrimUpscaler::GetSingleton()->mDisplaySizeY)
				return hr;
		}
		if (SettingGUI::GetSingleton()->sorted_item_list.size() > 0 && !REL::Module::IsVR())
			return hr;
		motion_item item = { 1u, *ppTexture2D, *pDesc };
		SettingGUI::GetSingleton()->sorted_item_list.push_back(item);
		SettingGUI::GetSingleton()->selected_item = item;
		creating = true;
		SkyrimUpscaler::GetSingleton()->SetupMotionVector(SettingGUI::GetSingleton()->selected_item.resource);
		creating = false;
		//SkyrimUpscaler::GetSingleton()->InitUpscaler();
		logger::info("Motion Vertor Found : {} x {}", pDesc->Width, pDesc->Height);
	} else if (pDesc->Format >= DXGI_FORMAT_R24G8_TYPELESS && pDesc->Format <= DXGI_FORMAT_X24_TYPELESS_G8_UINT) {
		if (pDesc->Width == SkyrimUpscaler::GetSingleton()->mDisplaySizeX &&
			pDesc->Height == SkyrimUpscaler::GetSingleton()->mDisplaySizeY &&
			pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL) {
			logger::info("Depth Buffer Found : {} x {}", pDesc->Width, pDesc->Height);
			static int VRSkip = REL::Module::IsVR() ? 3 : 0;
			if (VRSkip > 0) {
				VRSkip--;
				return hr;
			}
			if (SkyrimUpscaler::GetSingleton()->mDepthBuffer.mImage != nullptr)
				return hr;
			creating = true;
			SkyrimUpscaler::GetSingleton()->SetupDepth(*ppTexture2D);
			creating = false;
		}
	} else if (pDesc->Format == DXGI_FORMAT_R8G8B8A8_UNORM) {
		if (pDesc->Width == SkyrimUpscaler::GetSingleton()->mDisplaySizeX &&
			pDesc->Height == SkyrimUpscaler::GetSingleton()->mDisplaySizeY &&
			pDesc->BindFlags & (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET)) {
			if (SkyrimUpscaler::GetSingleton()->mTargetTex.mImage == nullptr)
				SkyrimUpscaler::GetSingleton()->SetupTarget(*ppTexture2D);
			//logger::info("Target Buffer Found : {} x {}  {}", pDesc->Width, pDesc->Height, size);
			targetMap[size] = *ppTexture2D;
			size++;
			index = size;
		}
	} else if (pDesc->Format == DXGI_FORMAT_R11G11B10_FLOAT) {
		if (pDesc->Width == SkyrimUpscaler::GetSingleton()->mDisplaySizeX &&
			pDesc->Height == SkyrimUpscaler::GetSingleton()->mDisplaySizeY &&
			pDesc->BindFlags & (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET)) {
			//if (SkyrimUpscaler::GetSingleton()->mTargetTex == nullptr)
			//	SkyrimUpscaler::GetSingleton()->SetupTarget(*ppTexture2D);
			logger::info("Target Buffer Found : {} x {}  {}", pDesc->Width, pDesc->Height, size2);
			targetMap2[size2] = *ppTexture2D;
			size2++;
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
	DXGI_SWAP_CHAIN_DESC sDesc;
	if (REL::Module::IsVR()) {
		memcpy(&sDesc, pSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
		sDesc.BufferDesc.Width = 1024;
		sDesc.BufferDesc.Height = 1024;
	}
	else
		sDesc = *pSwapChainDesc;

	logger::info("Calling original D3D11CreateDeviceAndSwapChain");
	HRESULT hr = (*ptrD3D11CreateDeviceAndSwapChain)(pAdapter,
		DriverType,
		Software,
		Flags,
		pFeatureLevels,
		FeatureLevels,
		SDKVersion,
		&sDesc,
		ppSwapChain,
		ppDevice,
		pFeatureLevel,
		ppImmediateContext);

	auto device = *ppDevice;
	auto deviceContext = *ppImmediateContext;
	auto swapChain = *ppSwapChain;
	SkyrimUpscaler::GetSingleton()->SetupSwapChain(swapChain);
	SkyrimUpscaler::GetSingleton()->PreInit();
	SettingGUI::GetSingleton()->InitIMGUI(swapChain, device, deviceContext);
	InitLogDelegate(MyLog);
	logger::info("Detouring virtual function tables");
	*(uintptr_t*)&ptrPresent = Detours::X64::DetourClassVTable(*(uintptr_t*)swapChain, &hk_IDXGISwapChain_Present, 8);
	*(uintptr_t*)&ptrCreateTexture2D = Detours::X64::DetourClassVTable(*(uintptr_t*)device, &hk_ID3D11Device_CreateTexture2D, 5);
	*(uintptr_t*)&ptrPSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_PSSetSamplers, 10);
	*(uintptr_t*)&ptrVSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_VSSetSamplers, 26);
	*(uintptr_t*)&ptrGSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_GSSetSamplers, 32);
	*(uintptr_t*)&ptrHSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_HSSetSamplers, 61);
	*(uintptr_t*)&ptrDSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_DSSetSamplers, 65);
	*(uintptr_t*)&ptrCSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_CSSetSamplers, 70);

	if (REL::Module::IsVR())
		*(uintptr_t*)&ptrOMSetRenderTargetsAndUnorderedAccessViews = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews, 34);

	return hr;
}

vr::EVRCompositorError hk_Submit(vr::IVRCompositor* This, vr::EVREye eEye, const vr::Texture_t* pTexture, const vr::VRTextureBounds_t* pBounds = 0, vr::EVRSubmitFlags nSubmitFlags = vr::Submit_Default)
{
	//logger::info("eEye = {}, pTexture= {}, uv=({},{},{},{})", (int)eEye, (uint32_t)pTexture->handle, pBounds->uMin, pBounds->uMax, pBounds->vMin, pBounds->vMax);
	static bool shift = false;
	if (GetAsyncKeyState(VK_SHIFT) < 0 && shift == false) {
		shift = true;
	}
	if (GetAsyncKeyState(VK_SHIFT) == 0 && shift == true) {
		shift = false;
		index--;
		if (index < 0)
			index = 0;
		logger::info("Setting target buffer to {} ", index);
	}
	static bool ctrl = false;
	if (GetAsyncKeyState(VK_CONTROL) < 0 && ctrl == false) {
		ctrl = true;
	}
	if (GetAsyncKeyState(VK_CONTROL) == 0 && ctrl == true) {
		ctrl = false;
		index++;
		if (index > size)
			index = size;
		logger::info("Setting target buffer to {} ", index);
	}
	vr::EVRCompositorError hr;
	if (index != size) {
		vr::Texture_t mTexture = {};
		mTexture.eColorSpace = pTexture->eColorSpace;
		mTexture.eType = pTexture->eType;
		mTexture.handle = targetMap[index];
		hr = (This->*ptrSubmit)(eEye, &mTexture, pBounds, nSubmitFlags);
	} else {
		hr = (This->*ptrSubmit)(eEye, pTexture, pBounds, nSubmitFlags);
	}
	return hr;
}

struct UpscalerHooks
{

	struct BSGraphics_Renderer_Init_InitD3D
	{
		static void thunk()
		{
			func();
			MenuOpenCloseEventHandler::Register();
			SkyrimUpscaler::GetSingleton()->InitUpscaler();
			//if (REL::Module::IsVR()) {
			//	vr::HmdError       HmdErr;
			//	vr::IVRCompositor* vrCompositor = (vr::IVRCompositor*)vr::VR_GetGenericInterface(vr::IVRCompositor_Version, &HmdErr);
			//	if (HmdErr != 0)
			//		logger::error("Getting IVRCompositor Failed ErrorCode: {}", HmdErr);
			//	else
			//		logger::info("Getting IVRCompositor Success!");
			//	*(uintptr_t*)&ptrSubmit = Detours::X64::DetourClassVTable(*(uintptr_t*)vrCompositor, &hk_Submit, 5);
			//}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct Main_DrawWorld_MainDraw
	{
		static void thunk(INT64 BSGraphics_Renderer, int unk)
		{
			if (SkyrimUpscaler::GetSingleton()->mTargetTex.mImage == nullptr)
				SkyrimUpscaler::GetSingleton()->SetupTarget(targetMap[24]);
			//static ID3D11RenderTargetView* targetRTV = nullptr;
			//if (targetMap[24] != nullptr) {
			//	if (targetRTV == nullptr)
			//		SkyrimUpscaler::GetSingleton()->mDevice->CreateRenderTargetView(targetMap[24], NULL, &targetRTV);
			//	if (targetRTV != nullptr) {
			//		const FLOAT color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
			//		SkyrimUpscaler::GetSingleton()->mContext->ClearRenderTargetView(targetRTV, color);
			//	}
			//}
			func(BSGraphics_Renderer, unk);
			SkyrimUpscaler::GetSingleton()->EvaluateUpscaler();
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct BSImagespaceShader_Hook_VR
	{
		static void thunk(RE::BSImagespaceShader* param_1, uint64_t param_2)
		{
			ID3D11RenderTargetView* RTV;
			ID3D11DepthStencilView* DSV;
			SkyrimUpscaler::GetSingleton()->mContext->OMGetRenderTargets(1, &RTV, &DSV);
			ID3D11Resource* renderTarget;
			RTV->GetResource(&renderTarget);
			ID3D11Texture2D* targetTex;
			renderTarget->QueryInterface(IID_PPV_ARGS(&targetTex));
			SkyrimUpscaler::GetSingleton()->SetupTarget(targetTex);
			static bool shift = false;
			if (GetAsyncKeyState(VK_SHIFT) < 0 && shift == false) {
				shift = true;
			}
			if (GetAsyncKeyState(VK_SHIFT) == 0 && shift == true) {
				shift = false;
				//DirectX::SaveWICTextureToFile(SkyrimUpscaler::GetSingleton()->mContext, renderTarget, GUID_ContainerFormatPng, L"TestVR.png");
				DRS::GetSingleton()->SetDRSVR();
			}
			flagSkipDrawCall = true;
			SkyrimUpscaler::GetSingleton()->EvaluateUpscaler();
			func(param_1, param_2);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct BSGraphics_Renderer_Begin_UpdateJitter
	{
		static void thunk(BSGraphics::State* a_state)
		{
			func(a_state);
			if (SkyrimUpscaler::GetSingleton()->IsEnabled() && 
				SkyrimUpscaler::GetSingleton()->mEnableJitter) {
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

	static void Install()
	{
		// Hook for getting the swapchain
		// Nope, depth and motion texture are already created after this function, so can't use it
		stl::write_thunk_call<BSGraphics_Renderer_Init_InitD3D>(REL::RelocationID(75595, 77226).address() + REL::Relocate(0x50, 0x2BC));
		// Have to hook the creation of SwapChain to hook CreateTexture2D before depth and motion textures are created
		char* ptr = nullptr;
		auto  moduleBase = (uintptr_t)GetModuleHandle(ptr);
		auto  dllD3D11 = GetModuleHandleA("d3d11.dll");
		*(FARPROC*)&ptrD3D11CreateDeviceAndSwapChain = GetProcAddress(dllD3D11, "D3D11CreateDeviceAndSwapChain");
		Detours::IATHook(moduleBase, "d3d11.dll", "D3D11CreateDeviceAndSwapChain", (uintptr_t)hk_D3D11CreateDeviceAndSwapChain);
		 
		// Setup our own jitters
		stl::write_thunk_call<BSGraphics_Renderer_Begin_UpdateJitter>(REL::RelocationID(75460, 77245).address() + REL::Relocate(0xE5, 0xE2, 0x104));  // D6A0C0 (D6A1A5), DA5A00 (DA5AE2)
		// Always enable TAA jitters, even without TAA
		static REL::Relocation<uintptr_t> updateJitterHook{ REL::RelocationID(75709, 77518) };          // D7CFB0, DB96E0
		static REL::Relocation<uintptr_t> buildCameraStateDataHook{ REL::RelocationID(75711, 77520) };  // D7D130, DB9850
		uint8_t                           patch1[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
		uint8_t                           patch2[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
		uint8_t                           patch3[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
		if (!REL::Module::IsVR()) {
			REL::safe_write<uint8_t>(updateJitterHook.address() + REL::Relocate(0xE, 0x11), patch1);
			REL::safe_write<uint8_t>(buildCameraStateDataHook.address() + REL::Relocate(0x1D5, 0x1D5), patch2);
			// Pre-UI Hook for upscaling, only for flatrim
			stl::write_thunk_call<Main_DrawWorld_MainDraw>(REL::RelocationID(79947, 82084).address() + REL::Relocate(0x16F, 0x17A, 0x132));  // EBF510 (EBF67F), F05BF0 (F05D6A)
		} else {
			REL::safe_write<uint8_t>(updateJitterHook.address() + 0x17, patch2);
			REL::safe_write<uint8_t>(buildCameraStateDataHook.address() + 0x34, patch3);
			// Pre-UI Hook for upscaling specifically for VR
			stl::write_thunk_call<BSImagespaceShader_Hook_VR>(REL::Offset(0x132c827).address());
		}
		logger::info("Installed upscaler hooks");
	}
};

void InstallUpscalerHooks() {
	UpscalerHooks::Install();
}

