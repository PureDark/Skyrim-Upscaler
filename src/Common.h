#pragma once

#include <d3d11.h>

#include <ReShade/reshade.hpp>
using namespace reshade::api;

#ifndef max
#	define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef SAFE_RELEASE
#	define SAFE_RELEASE(a) \
		if (a) {            \
			a->Release();   \
			a = NULL;       \
		}
#endif

const float Deg2Rad = 0.0174532924f;
const float Rad2Deg = 57.29578f;

enum UpscaleType
{
	DLSS,
	FSR2,
	XESS,
	DLAA,
	TAA
};

struct CustomConstants
{
	float jitterOffset[2];
	float dynamicResScale[2];
	float screenSize[2];
	float motionSensitivity;
	float blendScale;
	float leftRect[4];
	float rightRect[4];
};

struct FoveatedRect
{
	float left;
	float top;
	float right;
	float bottom;
};

struct ImageWrapper
{
public:
	ID3D11Texture2D*          mImage{ nullptr };
	ID3D11RenderTargetView*   mRTV{ nullptr };
	ID3D11ShaderResourceView* mSRV{ nullptr };
	ID3D11DepthStencilView*   mDSV{ nullptr };
	D3D11_TEXTURE2D_DESC      mDesc;
	ID3D11RenderTargetView*   GetRTV()
	{
		if (mImage != nullptr && mRTV == nullptr) {
			ID3D11Device* device;
			mImage->GetDevice(&device);
			HRESULT hr = device->CreateRenderTargetView(mImage, NULL, &mRTV);
			if (FAILED(hr))
				logger::error("Failed to Create RTV ErrorCode: 0x%08x", hr);
		}
		return mRTV;
	}
	ID3D11ShaderResourceView* GetSRV()
	{
		if (mImage != nullptr && mSRV == nullptr) {
			ID3D11Device* device;
			mImage->GetDevice(&device);
			D3D11_TEXTURE2D_DESC desc;
			mImage->GetDesc(&desc);
			HRESULT hr;
			if (desc.Format == DXGI_FORMAT_R24G8_TYPELESS) {
				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
				ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				srvDesc.Texture2D.MipLevels = 1;
				hr = device->CreateShaderResourceView(mImage, &srvDesc, &mSRV);
			} else if (desc.Format == DXGI_FORMAT_R16G16_TYPELESS) {
				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
				ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Format = DXGI_FORMAT_R16G16_UNORM;
				srvDesc.Texture2D.MipLevels = 1;
				hr = device->CreateShaderResourceView(mImage, &srvDesc, &mSRV);
			} else
				hr = device->CreateShaderResourceView(mImage, NULL, &mSRV);
			if (FAILED(hr))
				logger::error("Failed to Create SRV ErrorCode: 0x%08x", hr);
		}
		return mSRV;
	}
	ID3D11DepthStencilView* GetDSV()
	{
		if (mImage != nullptr && mDSV == nullptr) {
			ID3D11Device* device;
			mImage->GetDevice(&device);
			D3D11_DEPTH_STENCIL_VIEW_DESC desc;
			desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			desc.Flags = 0;
			desc.Texture2D.MipSlice = 0;
			HRESULT hr = device->CreateDepthStencilView(mImage, &desc, &mDSV);
			if (FAILED(hr))
				logger::error("Failed to Create DSV ErrorCode: 0x%08x", hr);
		}
		return mDSV;
	}
	D3D11_TEXTURE2D_DESC GetDesc()
	{
		if (mImage != nullptr ) {
			mImage->GetDesc(&mDesc);
		}
		return mDesc;
	}
	void Release()
	{
		if (mRTV) {
			mRTV->Release();
			mRTV = nullptr;
		}
		if (mSRV) {
			mSRV->Release();
			mSRV = nullptr;
		}
		if (mDSV) {
			mDSV->Release();
			mDSV = nullptr;
		}
		if (mImage) {
			mImage->Release();
			mImage = nullptr;
		}
	}
};
