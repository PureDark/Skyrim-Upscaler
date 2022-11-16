#include "d3d11_proxy.h"
#include <PCH.h>
#include <SettingGUI.h>
#include <DRS.h>
#include <SkyrimUpscaler.h>

//DXGISwapChainProxy::DXGISwapChainProxy(IDXGISwapChain* swapChain)
//{
//	HRESULT hr = swapChain->QueryInterface<IDXGISwapChain4>(&mSwapChain1);
//	ID3D11Device* device;
//	swapChain->GetDevice(IID_PPV_ARGS(&mD3d11Device));
//}
//DXGISwapChainProxy::DXGISwapChainProxy(IDXGISwapChain1* swapChain)
//{
//	HRESULT       hr = swapChain->QueryInterface<IDXGISwapChain4>(&mSwapChain1);
//	swapChain->GetDevice(IID_PPV_ARGS(&mD3d11Device));
//}
//DXGISwapChainProxy::DXGISwapChainProxy(IDXGISwapChain2* swapChain)
//{
//	HRESULT hr = swapChain->QueryInterface<IDXGISwapChain4>(&mSwapChain1);
//	swapChain->GetDevice(IID_PPV_ARGS(&mD3d11Device));
//}
//DXGISwapChainProxy::DXGISwapChainProxy(IDXGISwapChain3* swapChain)
//{
//	HRESULT hr = swapChain->QueryInterface<IDXGISwapChain4>(&mSwapChain1);
//	swapChain->GetDevice(IID_PPV_ARGS(&mD3d11Device));
//}
DXGISwapChainProxy::DXGISwapChainProxy(IDXGISwapChain* swapChain)
{
	mSwapChain1 = swapChain;
	swapChain->GetDevice(IID_PPV_ARGS(&mD3d11Device));
}

DXGISwapChainProxy::DXGISwapChainProxy()
{
}

void DXGISwapChainProxy::SetupSwapChain(IDXGISwapChain* swapChain)
{
	mSwapChain1 = swapChain;
	swapChain->GetDevice(IID_PPV_ARGS(&mD3d11Device));
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
	//D3D11_TEXTURE2D_DESC desc1;
	//back_buffer1->GetDesc(&desc1);
	ID3D11Texture2D* back_buffer2;
	mSwapChain2->GetBuffer(0, IID_PPV_ARGS(&back_buffer2));
	//D3D11_TEXTURE2D_DESC desc2;
	//back_buffer2->GetDesc(&desc2);
	//logger::info("Buffer 1 : {} x {}", desc1.Width, desc1.Height);
	//logger::info("Buffer 2 : {} x {}", desc2.Width, desc2.Height);
	hr = mSwapChain2->Present(SyncInterval, Flags);
	//ID3D11DeviceContext* context;
	//mD3d11Device->GetImmediateContext(&context);
	//context->CopyResource(back_buffer1, back_buffer2);
	SkyrimUpscaler::GetSingleton()->ForceEvaluateUpscaler(back_buffer2, back_buffer1);
	hr = mSwapChain1->Present(SyncInterval, Flags);

	//usingSwapChain2 = true;
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

///****IDXGISwapChain1****/
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetDesc1(_Out_ DXGI_SWAP_CHAIN_DESC1* pDesc)
//{
//	return GetCurrentSwapChain()->GetDesc1(pDesc);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetFullscreenDesc(_Out_ DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc)
//{
//	return GetCurrentSwapChain()->GetFullscreenDesc(pDesc);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetHwnd(_Out_ HWND* pHwnd)
//{
//	return GetCurrentSwapChain()->GetHwnd(pHwnd);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetCoreWindow(_In_ REFIID refiid, _COM_Outptr_ void** ppUnk)
//{
//	return GetCurrentSwapChain()->GetCoreWindow(refiid, ppUnk);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::Present1(UINT SyncInterval, UINT PresentFlags, _In_ const DXGI_PRESENT_PARAMETERS* pPresentParameters)
//{
//	return GetCurrentSwapChain()->Present1(SyncInterval, PresentFlags, pPresentParameters);
//}
//
//BOOL STDMETHODCALLTYPE DXGISwapChainProxy::IsTemporaryMonoSupported(void)
//{
//	return GetCurrentSwapChain()->IsTemporaryMonoSupported();
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetRestrictToOutput(_Out_ IDXGIOutput** ppRestrictToOutput)
//{
//	return GetCurrentSwapChain()->GetRestrictToOutput(ppRestrictToOutput);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::SetBackgroundColor(_In_ const DXGI_RGBA* pColor)
//{
//	return GetCurrentSwapChain()->SetBackgroundColor(pColor);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetBackgroundColor(_Out_ DXGI_RGBA* pColor)
//{
//	return GetCurrentSwapChain()->GetBackgroundColor(pColor);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::SetRotation(_In_ DXGI_MODE_ROTATION Rotation)
//{
//	return GetCurrentSwapChain()->SetRotation(Rotation);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetRotation(_Out_ DXGI_MODE_ROTATION* pRotation)
//{
//	return GetCurrentSwapChain()->GetRotation(pRotation);
//}
//
//
///****IDXGISwapChain2****/
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::SetSourceSize(UINT Width, UINT Height)
//{
//	return GetCurrentSwapChain()->SetSourceSize(Width, Height);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetSourceSize(_Out_ UINT* pWidth, _Out_ UINT* pHeight)
//{
//	return GetCurrentSwapChain()->GetSourceSize(pWidth, pHeight);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::SetMaximumFrameLatency(UINT MaxLatency)
//{
//	return GetCurrentSwapChain()->SetMaximumFrameLatency(MaxLatency);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetMaximumFrameLatency(_Out_ UINT* pMaxLatency)
//{
//	return GetCurrentSwapChain()->GetMaximumFrameLatency(pMaxLatency);
//}
//
//HANDLE STDMETHODCALLTYPE DXGISwapChainProxy::GetFrameLatencyWaitableObject(void)
//{
//	return GetCurrentSwapChain()->GetFrameLatencyWaitableObject();
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::SetMatrixTransform(const DXGI_MATRIX_3X2_F* pMatrix)
//{
//	return GetCurrentSwapChain()->SetMatrixTransform(pMatrix);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::GetMatrixTransform(_Out_ DXGI_MATRIX_3X2_F* pMatrix)
//{
//	return GetCurrentSwapChain()->GetMatrixTransform(pMatrix);
//}
//
///****IDXGISwapChain3****/
//UINT STDMETHODCALLTYPE DXGISwapChainProxy::GetCurrentBackBufferIndex(void)
//{
//	return GetCurrentSwapChain()->GetCurrentBackBufferIndex();
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::CheckColorSpaceSupport(_In_ DXGI_COLOR_SPACE_TYPE ColorSpace, _Out_ UINT* pColorSpaceSupport)
//{
//	return GetCurrentSwapChain()->CheckColorSpaceSupport(ColorSpace, pColorSpaceSupport);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::SetColorSpace1(_In_ DXGI_COLOR_SPACE_TYPE ColorSpace)
//{
//	return GetCurrentSwapChain()->SetColorSpace1(ColorSpace);
//}
//
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::ResizeBuffers1(_In_ UINT BufferCount, _In_ UINT Width, _In_ UINT Height, _In_ DXGI_FORMAT Format,
//	_In_ UINT SwapChainFlags, _In_reads_(BufferCount) const UINT* pCreationNodeMask, _In_reads_(BufferCount) IUnknown* const* ppPresentQueue)
//{
//	return GetCurrentSwapChain()->ResizeBuffers1(BufferCount, Width, Height, Format, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
//}
//
///****IDXGISwapChain4****/
//HRESULT STDMETHODCALLTYPE DXGISwapChainProxy::SetHDRMetaData(_In_ DXGI_HDR_METADATA_TYPE Type, _In_ UINT Size, _In_reads_opt_(Size) void* pMetaData)
//{
//	return GetCurrentSwapChain()->SetHDRMetaData(Type, Size, pMetaData);
//}
