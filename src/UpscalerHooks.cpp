#pragma once
#include <d3d11.h>
#include <dxgi.h>

#include <Detours.h>
#include "SkyrimUpscaler.h"
#include "SettingGUI.h"

#include <Detours.h>
#include "DRS.h"

using namespace BSGraphics;

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
//decltype(&ID3D11Device::CreateTexture2D)      ptrCreateTexture2D;
decltype(&ID3D11DeviceContext::PSSetSamplers) ptrPSSetSamplers;
decltype(&ID3D11DeviceContext::VSSetSamplers) ptrVSSetSamplers;
decltype(&ID3D11DeviceContext::GSSetSamplers) ptrGSSetSamplers;
decltype(&ID3D11DeviceContext::HSSetSamplers) ptrHSSetSamplers;
decltype(&ID3D11DeviceContext::DSSetSamplers) ptrDSSetSamplers;
decltype(&ID3D11DeviceContext::CSSetSamplers) ptrCSSetSamplers;

decltype(&ID3D11DeviceContext::OMSetRenderTargets) ptrOMSetRenderTargets;
decltype(&ID3D11DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews) ptrOMSetRenderTargetsAndUnorderedAccessViews;

static auto upscaler = SkyrimUpscaler::GetSingleton();

static void MyLog(char* message, int size)
{
	logger::info("{}", message);
}

void WINAPI hk_ID3D11DeviceContext_OMSetRenderTargets(ID3D11DeviceContext* This, UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView)
{
	(This->*ptrOMSetRenderTargets)(NumViews, ppRenderTargetViews, pDepthStencilView);
	if (upscaler->mVRS)
		upscaler->mVRS->PostOMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
}

void WINAPI hk_ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews(ID3D11DeviceContext* This, UINT NumRTVs, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView, 
	UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView* const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts)
{
	(This->*ptrOMSetRenderTargetsAndUnorderedAccessViews)(NumRTVs, ppRenderTargetViews, pDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
	if (upscaler->mVRS)
		upscaler->mVRS->PostOMSetRenderTargets(NumRTVs, ppRenderTargetViews, pDepthStencilView);
}


HRESULT WINAPI hk_IDXGISwapChain_Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	SettingGUI::GetSingleton()->OnRender();
	auto hr = (This->*ptrPresent)(SyncInterval, Flags);
	DRS::GetSingleton()->Update();

	
	if (auto r = BSGraphics::Renderer::QInstance()) {
		for (int i = 0; i < RENDER_TARGET_COUNT; i++) {
			auto rt = r->pRenderTargets[i];
			if (rt.Texture != NULL && (rt.Texture == upscaler->mOpaqueColor.mImage || rt.Texture == upscaler->mOpaqueColorHDR.mImage)) {
				bool a = true;
			}
			if (rt.TextureCopy != NULL && (rt.TextureCopy == upscaler->mOpaqueColor.mImage || rt.TextureCopy == upscaler->mOpaqueColorHDR.mImage)) {
				bool b = true;
			}
		}
	}
	return hr;
}

// Mostly from vrperfkit, thanks to fholger for showing how to do mip lod bias
// https://github.com/fholger/vrperfkit/blob/037c09f3168ac045b5775e8d1a0c8ac982b5854f/src/d3d11/d3d11_post_processor.cpp#L76
static void SetMipLodBias(ID3D11SamplerState** outSamplers, UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)
{
	static int Skip = 0;
	if (mipLodBias != upscaler->mMipLodBias) {
		logger::info("MIP LOD Bias changed from  {} to {}, recreating samplers", mipLodBias, upscaler->mMipLodBias);
		passThroughSamplers.clear();
		mappedSamplers.clear();
		mipLodBias = upscaler->mMipLodBias;
		Skip = 1;
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
			if (sd.MipLODBias != 0) {
				passThroughSamplers.insert(orig);
				continue;
			}
			if (sd.MaxAnisotropy <= 1 && Skip-- != 1) {
				passThroughSamplers.insert(orig);
				continue;
			}
			sd.MipLODBias = mipLodBias;

			upscaler->mDevice->CreateSamplerState(&sd, &mappedSamplers[orig]);
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
	memcpy(&sDesc, pSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	sDesc.BufferDesc.Width = 1024;
	sDesc.BufferDesc.Height = 1024;

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
	IDXGISwapChain2* unwrappedSwapChain;
	swapChain->QueryInterface(IID_PPV_ARGS(&unwrappedSwapChain));
	upscaler->SetupSwapChain(unwrappedSwapChain);
	upscaler->PreInit();
	SettingGUI::GetSingleton()->InitIMGUI(swapChain, device, deviceContext);
	InitLogDelegate(MyLog);
	logger::info("Detouring virtual function tables");
	*(uintptr_t*)&ptrPresent = Detours::X64::DetourClassVTable(*(uintptr_t*)swapChain, &hk_IDXGISwapChain_Present, 8);
	//*(uintptr_t*)&ptrCreateTexture2D = Detours::X64::DetourClassVTable(*(uintptr_t*)device, &hk_ID3D11Device_CreateTexture2D, 5);
	*(uintptr_t*)&ptrPSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_PSSetSamplers, 10);
	*(uintptr_t*)&ptrVSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_VSSetSamplers, 26);
	*(uintptr_t*)&ptrGSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_GSSetSamplers, 32);
	*(uintptr_t*)&ptrHSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_HSSetSamplers, 61);
	*(uintptr_t*)&ptrDSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_DSSetSamplers, 65);
	*(uintptr_t*)&ptrCSSetSamplers = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_CSSetSamplers, 70);

	*(uintptr_t*)&ptrOMSetRenderTargets = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_OMSetRenderTargets, 33);
	*(uintptr_t*)&ptrOMSetRenderTargetsAndUnorderedAccessViews = Detours::X64::DetourClassVTable(*(uintptr_t*)deviceContext, &hk_ID3D11DeviceContext_OMSetRenderTargetsAndUnorderedAccessViews, 34);

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

			static auto  renderer = RE::BSGraphics::Renderer::GetSingleton();
			static auto& depthTexture = renderer->GetDepthStencilData().depthStencils[RE::RENDER_TARGETS_DEPTHSTENCIL::kPOST_ZPREPASS_COPY];
			static auto& motionVectorsTexture = renderer->GetRuntimeData().renderTargets[RE::RENDER_TARGET::kMOTION_VECTOR];
			upscaler->InitUpscaler();
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct BSImagespaceShader_Hook_VR
	{
		static void thunk(RE::BSImagespaceShader* param_1, uint64_t param_2)
		{
			if (upscaler->mNeedUpdate || 
				upscaler->mTargetTex.mImage == nullptr) {
				upscaler->mNeedUpdate = false;
				//UnkOuterStruct::GetSingleton()->SetTAA(upscaler->mUseTAAForPeriphery);
				//ID3D11RenderTargetView* RTV;
				//ID3D11DepthStencilView* DSV;
				//upscaler->mContext->OMGetRenderTargets(1, &RTV, &DSV);
				//ID3D11Resource* resource;
				//RTV->GetResource(&resource);
				//upscaler->SetupTarget((ID3D11Texture2D*)resource);
				//upscaler->mTargetTex.mRTV = RTV;
			}

			UnkOuterStruct::GetSingleton()->SetTAA(upscaler->mUseTAAForPeriphery);
			ID3D11RenderTargetView* RTV;
			ID3D11DepthStencilView* DSV;
			upscaler->mContext->OMGetRenderTargets(1, &RTV, &DSV);
			ID3D11Resource* resource;
			RTV->GetResource(&resource);
			upscaler->SetupTarget((ID3D11Texture2D*)resource);
			upscaler->mTargetTex.mRTV = RTV;

			func(param_1, param_2);
			static ImageWrapper             SourceTex{ nullptr };
			static ImageWrapper             TargetTex{ nullptr };
			static ImageWrapper             DepthTex{ nullptr };
			if (TargetTex.mImage == nullptr) {
				ID3D11RenderTargetView* RTV = nullptr;
				ID3D11DepthStencilView* DSV = nullptr;
				upscaler->mContext->OMGetRenderTargets(1, &RTV, &DSV);
				if (RTV != nullptr) {
					ID3D11Resource* resource;
					RTV->GetResource(&resource);
					TargetTex.mImage = (ID3D11Texture2D*)resource;
					TargetTex.mRTV = RTV;
				}
				if (DSV != nullptr) {
					ID3D11Resource* resource;
					DSV->GetResource(&resource);
					DepthTex.mImage = (ID3D11Texture2D*)resource;
					DepthTex.mDSV = DSV;
				}
				ID3D11ShaderResourceView* SRV;
				upscaler->mContext->PSGetShaderResources(0, 1, &SRV);
				ID3D11Resource* resource;
				SRV->GetResource(&resource);
				SourceTex.mImage = (ID3D11Texture2D*)resource;
			}
			//if (upscaler->m_runtime != nullptr && !upscaler->mUseTAAForPeriphery) {
			//	if (upscaler->m_rtv.handle == 0)
			//		upscaler->m_rtv = { reinterpret_cast<uintptr_t>(upscaler->mTargetTex.mRTV) };
			//	upscaler->m_runtime->render_effects(upscaler->m_runtime->get_command_queue()->get_immediate_command_list(), upscaler->m_rtv, upscaler->m_rtv);
			//}
			
			if (auto r = BSGraphics::Renderer::QInstance()) {
				auto rt = r->pRenderTargets[BSGraphics::RenderTargets::RENDER_TARGET_MOTION_VECTOR];
				upscaler->SetupMotionVector(rt.Texture);
				auto depthRt = r->pDepthStencils[BSGraphics::RenderTargetsDepthStencil::DEPTH_STENCIL_TARGET_MAIN];
				if (depthRt.Texture) {
					upscaler->SetupDepth(depthRt.Texture);
				}
			}
			if (upscaler->mUpscaleDepthForReShade) {
				upscaler->mContext->CopyResource(upscaler->mDepthBuffer.mImage, DepthTex.mImage);
			}
			upscaler->Evaluate(TargetTex.mImage, DepthTex.mDSV);
			upscaler->DelayEnable();
			float color[4] = { 0, 0, 0, 1 };
			upscaler->mContext->ClearRenderTargetView(SourceTex.GetRTV(), color);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct Hook_PreTAA
	{
		static void thunk(INT64* a1, unsigned int a2, unsigned int a3)
		{
			static ImageWrapper beforeTAAImage;
			if (beforeTAAImage.mImage == nullptr) {
				ID3D11Resource*  beforeTAATex = nullptr;
				ID3D11RenderTargetView* RTV;
				ID3D11DepthStencilView* DSV;
				upscaler->mContext->OMGetRenderTargets(1, &RTV, &DSV);
				RTV->GetResource(&beforeTAATex);
				beforeTAAImage.mImage = (ID3D11Texture2D*)beforeTAATex;
				beforeTAAImage.mRTV = RTV;
			}
			//if (upscaler->m_runtime != nullptr) {
			//	if (upscaler->m_rtv.handle == 0)
			//		upscaler->m_rtv = { reinterpret_cast<uintptr_t>(beforeTAAImage.mRTV) };
			//	upscaler->m_runtime->render_effects(upscaler->m_runtime->get_command_queue()->get_immediate_command_list(), upscaler->m_rtv, upscaler->m_rtv);
			//}
			upscaler->mContext->CopyResource(upscaler->mTempColor.mImage, beforeTAAImage.mImage);
			func(a1, a2, a3);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct BSGraphics_Renderer_Begin_UpdateJitter
	{
		static void thunk(BSGraphics::State* a_state)
		{
			func(a_state);
			if (upscaler->IsEnabled() && 
				upscaler->mEnableJitter) {
				if (upscaler->mUpscaleType != TAA) {
					float x = 0.0f;
					float y = 0.0f;
					upscaler->GetJitters(&x, &y);
					float w = upscaler->mRenderSizeX;
					float h = upscaler->mRenderSizeY;
					a_state->jitter[0] = -4 * x / w;
					a_state->jitter[1] = 2 * y / h;
					upscaler->SetJitterOffsets(-x, -y);
				}
			} else {
				a_state->jitter[0] = 0;
				a_state->jitter[1] = 0;
				upscaler->SetJitterOffsets(0, 0);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	static void Install()
	{
		if (REL::Module::IsVR()) {
			// Hook for getting the swapchain
			// Nope, depth and motion texture are already created after this function, so can't use it
			stl::write_thunk_call<BSGraphics_Renderer_Init_InitD3D>(REL::RelocationID(75595, 77226).address() + REL::Relocate(0x50, 0x2BC));
			// Have to hook the creation of SwapChain to hook CreateTexture2D before depth and motion textures are created
			char* ptr = nullptr;
			auto  moduleBase = (uintptr_t)GetModuleHandle(ptr);
			auto  dllD3D11 = GetModuleHandleA("d3d11.dll");
			//*(FARPROC*)&ptrD3D11CreateDeviceAndSwapChain = GetProcAddress(dllD3D11, "D3D11CreateDeviceAndSwapChain");
			*(void**)&ptrD3D11CreateDeviceAndSwapChain = (uintptr_t*)Detours::IATHook(moduleBase, "d3d11.dll", "D3D11CreateDeviceAndSwapChain", (uintptr_t)hk_D3D11CreateDeviceAndSwapChain);

			// Setup our own jitters
			stl::write_thunk_call<BSGraphics_Renderer_Begin_UpdateJitter>(REL::RelocationID(75460, 77245).address() + REL::Relocate(0xE5, 0xE2, 0x104));  // D6A0C0 (D6A1A5), DA5A00 (DA5AE2)
			// Always enable TAA jitters, even without TAA
			static REL::Relocation<uintptr_t> updateJitterHook{ REL::RelocationID(75709, 77518) };          // D7CFB0, DB96E0
			static REL::Relocation<uintptr_t> buildCameraStateDataHook{ REL::RelocationID(75711, 77520) };  // D7D130, DB9850
			uint8_t                           patch1[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
			uint8_t                           patch2[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
			uint8_t                           patch3[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
			REL::safe_write<uint8_t>(updateJitterHook.address() + 0x17, patch2);
			REL::safe_write<uint8_t>(buildCameraStateDataHook.address() + 0x34, patch3);
			// Pre-UI Hook for upscaling specifically for VR
			stl::write_thunk_call<BSImagespaceShader_Hook_VR>(REL::Offset(0x132c827).address());
			// Pre-TAA Hook for VR
			stl::write_thunk_call<Hook_PreTAA>(REL::Offset(0x12D1F0C).address());
		}
		logger::info("Installed upscaler hooks");
	}
};

void InstallUpscalerHooks() {
	UpscalerHooks::Install();
}

