#include "d3d11_proxy.h"
#include <PCH.h>
#include <SettingGUI.h>
#include <DRS.h>
#include <SkyrimUpscaler.h>
#include <ScreenGrab11.h>
#include <wincodec.h>
#include <d3dcompiler.h>
#include <hlsl/flip.vs.inc>
#include <hlsl/Bicubic.inc>

DXGISwapChainProxy::DXGISwapChainProxy(IDXGISwapChain* swapChain)
{
	mSwapChain1 = swapChain;
	swapChain->GetDevice(IID_PPV_ARGS(&mDevice));
	mDevice->GetImmediateContext(&mContext);
	InitShader();
}

IDXGISwapChain* DXGISwapChainProxy::GetCurrentSwapChain(){
	return (usingSwapChain2) ? mSwapChain2 : mSwapChain1;
}

/****IUknown****/
HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::QueryInterface(REFIID riid, void** ppvObj)
{
	if (riid == __uuidof(IDXGISwapChain) || riid == __uuidof(IDXGISwapChain2) || riid == __uuidof(IDXGISwapChain3) || riid == __uuidof(IDXGISwapChain4))
		return GetCurrentSwapChain()->QueryInterface(riid, ppvObj);

	return E_NOTIMPL;
}

ULONG STDMETHODCALLTYPE DXGISwapChainProxy::AddRef()
{
	return GetCurrentSwapChain()->AddRef();
}

ULONG STDMETHODCALLTYPE DXGISwapChainProxy::Release()
{
	return GetCurrentSwapChain()->Release();
}

/****IDXGIObject****/
HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::SetPrivateData(_In_ REFGUID Name, UINT DataSize, _In_reads_bytes_(DataSize) const void* pData)
{
	return GetCurrentSwapChain()->SetPrivateData(Name, DataSize, pData);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::SetPrivateDataInterface(_In_ REFGUID Name, _In_opt_ const IUnknown* pUnknown)
{
	return GetCurrentSwapChain()->SetPrivateDataInterface(Name, pUnknown);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetPrivateData(_In_ REFGUID Name, _Inout_ UINT* pDataSize, _Out_writes_bytes_(*pDataSize) void* pData)
{
	return GetCurrentSwapChain()->GetPrivateData(Name, pDataSize, pData);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetParent(_In_ REFIID riid, _COM_Outptr_ void** ppParent)
{
	return GetCurrentSwapChain()->GetParent(riid, ppParent);
}

/****IDXGIDeviceSubObject****/
HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetDevice(_In_ REFIID riid, _COM_Outptr_ void** ppDevice)
{
	return GetCurrentSwapChain()->GetDevice(riid, ppDevice);
}

/****IDXGISwapChain****/
HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::Present(UINT SyncInterval, UINT Flags)
{
	SettingGUI::GetSingleton()->OnRender();
	DRS::GetSingleton()->Update();
	HRESULT hr;
	ID3D11Texture2D* back_buffer1;
	mSwapChain1->GetBuffer(0, IID_PPV_ARGS(&back_buffer1));
	ID3D11Texture2D* back_buffer2;
	mSwapChain2->GetBuffer(0, IID_PPV_ARGS(&back_buffer2));
	// Call Present to inform ENB, but we intercept it to not do actual present
	hr = mSwapChain2->Present(SyncInterval, Flags);

	if (ImGui::IsKeyReleased(ImGuiKey_Keypad0)) {
		DirectX::SaveWICTextureToFile(mContext, back_buffer2, GUID_ContainerFormatPng, L"test2.png");
	}

	static ID3D11RenderTargetView* backBufferView = nullptr;
	if (backBufferView == nullptr)
	    mDevice->CreateRenderTargetView(back_buffer1, NULL, &backBufferView);

	static ID3D11ShaderResourceView* shaderResourceView = nullptr;
	if (shaderResourceView == nullptr)
		mDevice->CreateShaderResourceView(back_buffer2, NULL, &shaderResourceView);
	RenderTexture(shaderResourceView, backBufferView, SkyrimUpscaler::GetSingleton()->mDisplaySizeX, SkyrimUpscaler::GetSingleton()->mDisplaySizeY);

	hr = mSwapChain1->Present(SyncInterval, Flags);
	return hr;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetBuffer(UINT Buffer, _In_ REFIID riid, _COM_Outptr_ void** ppSurface)
{
	return GetCurrentSwapChain()->GetBuffer(Buffer, riid, ppSurface);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::SetFullscreenState(BOOL Fullscreen, _In_opt_ IDXGIOutput* pTarget)
{
	return GetCurrentSwapChain()->SetFullscreenState(Fullscreen, pTarget);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetFullscreenState(_Out_opt_ BOOL* pFullscreen, _COM_Outptr_opt_result_maybenull_ IDXGIOutput** ppTarget)
{
	return GetCurrentSwapChain()->GetFullscreenState(pFullscreen, ppTarget);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetDesc(_Out_ DXGI_SWAP_CHAIN_DESC* pDesc)
{
	return GetCurrentSwapChain()->GetDesc(pDesc);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	return GetCurrentSwapChain()->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::ResizeTarget(_In_ const DXGI_MODE_DESC* pNewTargetParameters)
{
	return GetCurrentSwapChain()->ResizeTarget(pNewTargetParameters);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetContainingOutput(_COM_Outptr_ IDXGIOutput** ppOutput)
{
	return GetCurrentSwapChain()->GetContainingOutput(ppOutput);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetFrameStatistics(_Out_ DXGI_FRAME_STATISTICS* pStats)
{
	return GetCurrentSwapChain()->GetFrameStatistics(pStats);
}

HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetLastPresentCount(_Out_ UINT* pLastPresentCount)
{
	return GetCurrentSwapChain()->GetLastPresentCount(pLastPresentCount);
}



void DXGISwapChainProxy::InitShader()
{
	mDevice->CreateVertexShader(VS_Flip, sizeof(VS_Flip), nullptr, &mVertexShader);
	mDevice->CreatePixelShader(Bicubic, sizeof(Bicubic), nullptr, &mPixelShader);

	D3D11_SAMPLER_DESC sd;
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.MipLODBias = 0;
	sd.MaxAnisotropy = 1;
	sd.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sd.MinLOD = 0;
	sd.MaxLOD = 0;
	mDevice->CreateSamplerState(&sd, &mSampler);

	D3D11_RASTERIZER_DESC rd;
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE;
	rd.FrontCounterClockwise = TRUE;
	rd.DepthBias = 0;
	rd.DepthBiasClamp = 0;
	rd.SlopeScaledDepthBias = 0;
	rd.DepthClipEnable = FALSE;
	rd.ScissorEnable = FALSE;
	rd.MultisampleEnable = FALSE;
	rd.AntialiasedLineEnable = FALSE;
	mDevice->CreateRasterizerState(&rd, &mRasterizerState);

	D3D11_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(blendDesc));
	auto& rtDesc = blendDesc.RenderTarget[0];

	// Color = SrcAlpha * SrcColor + (1 - SrcAlpha) * DestColor
	// Alpha = SrcAlpha
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	rtDesc.BlendEnable = true;
	rtDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	rtDesc.BlendOp = D3D11_BLEND_OP_ADD;
	rtDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	mDevice->CreateBlendState(&blendDesc, &mBlendState);
}

void DXGISwapChainProxy::RenderTexture(ID3D11ShaderResourceView* sourceTexture, ID3D11RenderTargetView* target, int width, int height)
{
	mContext->OMSetRenderTargets(1, &target, nullptr);
	mContext->OMSetBlendState(mBlendState, nullptr, 0xffffffff);
	mContext->OMSetDepthStencilState(nullptr, 0);
	mContext->VSSetShader(mVertexShader, nullptr, 0);
	mContext->PSSetShader(mPixelShader, nullptr, 0);
	mContext->PSSetShaderResources(0, 1, &sourceTexture);
	mContext->PSSetSamplers(0, 1, &mSampler);
	mContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	mContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	mContext->IASetInputLayout(nullptr);
	mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	D3D11_VIEWPORT vp;
	vp.TopLeftX = vp.TopLeftY = 0;
	vp.Width = width;
	vp.Height = height;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	mContext->RSSetViewports(1, &vp);
	mContext->RSSetState(mRasterizerState);

	mContext->Draw(3, 0);

	mContext->OMSetRenderTargets(0, nullptr, nullptr);
	mContext->PSSetShaderResources(0, 0, nullptr);
}
