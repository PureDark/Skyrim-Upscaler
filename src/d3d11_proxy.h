#pragma once

#include <d3d11.h>

struct DXGISwapChainProxy;
struct DXGISwapChainProxy : IDXGISwapChain
{
public:
	bool                   usingSwapChain2 = false;
	IDXGISwapChain*        mSwapChain1{ nullptr };
	IDXGISwapChain*        mSwapChain2{ nullptr };
	ID3D11Device*          mDevice{ nullptr };
	ID3D11DeviceContext*   mContext{ nullptr };
	ID3D11VertexShader*    mVertexShader{ nullptr };
	ID3D11PixelShader*     mPixelShader{ nullptr };
	ID3D11SamplerState*    mSampler{ nullptr };
	ID3D11RasterizerState* mRasterizerState{ nullptr };
	ID3D11BlendState*      mBlendState{ nullptr };

	DXGISwapChainProxy(IDXGISwapChain* swapChain);

	IDXGISwapChain* GetCurrentSwapChain();

	void InitShader();

	void RenderTexture(ID3D11ShaderResourceView* sourceTexture, ID3D11RenderTargetView* target, int width, int height);

	/****IUnknown****/
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
	virtual ULONG STDMETHODCALLTYPE   AddRef() override;
	virtual ULONG STDMETHODCALLTYPE   Release() override;

	
	/****IDXGIObject****/
	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(_In_ REFGUID Name, UINT DataSize, _In_reads_bytes_(DataSize) const void* pData) override;

	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(_In_ REFGUID Name, _In_opt_ const IUnknown* pUnknown) override;

	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(_In_ REFGUID Name, _Inout_ UINT* pDataSize, _Out_writes_bytes_(*pDataSize) void* pData) override;

	virtual HRESULT STDMETHODCALLTYPE GetParent(_In_ REFIID riid, _COM_Outptr_ void** ppParent) override;

	/****IDXGIDeviceSubObject****/
	virtual HRESULT STDMETHODCALLTYPE GetDevice(_In_ REFIID riid, _COM_Outptr_ void** ppDevice) override;

	/****IDXGISwapChain****/
	virtual HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags);

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, _In_ REFIID riid, _COM_Outptr_ void** ppSurface);

	virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, _In_opt_ IDXGIOutput* pTarget);

	virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(_Out_opt_ BOOL* pFullscreen, _COM_Outptr_opt_result_maybenull_ IDXGIOutput** ppTarget);

	virtual HRESULT STDMETHODCALLTYPE GetDesc(_Out_ DXGI_SWAP_CHAIN_DESC* pDesc);

	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

	virtual HRESULT STDMETHODCALLTYPE ResizeTarget(_In_ const DXGI_MODE_DESC* pNewTargetParameters);

	virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(_COM_Outptr_ IDXGIOutput** ppOutput);

	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(_Out_ DXGI_FRAME_STATISTICS* pStats);

	virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(_Out_ UINT* pLastPresentCount);

	///****IDXGISwapChain1****/
	//virtual HRESULT STDMETHODCALLTYPE GetDesc1(_Out_ DXGI_SWAP_CHAIN_DESC1* pDesc);

	//virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc(_Out_ DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc);

	//virtual HRESULT STDMETHODCALLTYPE GetHwnd(_Out_ HWND* pHwnd);

	//virtual HRESULT STDMETHODCALLTYPE GetCoreWindow(_In_ REFIID refiid, _COM_Outptr_ void** ppUnk);

	//virtual HRESULT STDMETHODCALLTYPE Present1(UINT SyncInterval, UINT PresentFlags, _In_ const DXGI_PRESENT_PARAMETERS* pPresentParameters);

	//virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported(void);

	//virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput(_Out_ IDXGIOutput** ppRestrictToOutput);

	//virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(_In_ const DXGI_RGBA* pColor);

	//virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(_Out_ DXGI_RGBA* pColor);

	//virtual HRESULT STDMETHODCALLTYPE SetRotation(_In_ DXGI_MODE_ROTATION Rotation);

	//virtual HRESULT STDMETHODCALLTYPE GetRotation(_Out_ DXGI_MODE_ROTATION* pRotation);
	//
	///****IDXGISwapChain2****/
 //   virtual HRESULT STDMETHODCALLTYPE SetSourceSize(UINT Width, UINT Height);

	//virtual HRESULT STDMETHODCALLTYPE GetSourceSize(_Out_ UINT* pWidth, _Out_ UINT* pHeight);

	//virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency);

	//virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(_Out_ UINT* pMaxLatency);

	//virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject(void);

	//virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F* pMatrix);

	//virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform(_Out_ DXGI_MATRIX_3X2_F* pMatrix);

	///****IDXGISwapChain3****/
	//virtual UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex(void);

	//virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(_In_ DXGI_COLOR_SPACE_TYPE ColorSpace, _Out_ UINT* pColorSpaceSupport);

	//virtual HRESULT STDMETHODCALLTYPE SetColorSpace1(_In_ DXGI_COLOR_SPACE_TYPE ColorSpace);

	//virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1(_In_ UINT BufferCount, _In_ UINT Width, _In_ UINT Height, _In_ DXGI_FORMAT Format, 
	//	_In_ UINT SwapChainFlags, _In_reads_(BufferCount) const UINT* pCreationNodeMask, _In_reads_(BufferCount) IUnknown* const* ppPresentQueue);

	///****IDXGISwapChain4****/
	//virtual HRESULT STDMETHODCALLTYPE SetHDRMetaData(_In_ DXGI_HDR_METADATA_TYPE Type, _In_ UINT Size, _In_reads_opt_(Size) void* pMetaData);
};
