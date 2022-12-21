#pragma once

#include <string>
#include <d3d11.h>
#include <wrl/client.h>
#include "nvapi.h"

struct ImageWrapper
{
public:
	ID3D11Texture2D*          mImage{ nullptr };
	ID3D11RenderTargetView*   mRTV{ nullptr };
	ID3D11ShaderResourceView* mSRV{ nullptr };
	ID3D11DepthStencilView*   mDSV{ nullptr };
	ID3D11RenderTargetView*   GetRTV()
	{
		if (mImage != nullptr && mRTV == nullptr) {
			ID3D11Device* device;
			mImage->GetDevice(&device);
			device->CreateRenderTargetView(mImage, NULL, &mRTV);
		}
		return mRTV;
	}
	ID3D11ShaderResourceView* GetSRV()
	{
		if (mImage != nullptr && mSRV == nullptr) {
			ID3D11Device* device;
			mImage->GetDevice(&device);
			device->CreateShaderResourceView(mImage, NULL, &mSRV);
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
			device->CreateDepthStencilView(mImage, &desc, &mDSV);
		}
		return mDSV;
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

class D3D11VariableRateShading
{
public:
	bool  mEnableFixedFoveatedRendering = false;
	float mInnerRadius = 0.6f;
	float mMiddleRadius = 0.8f;
	float mOutterRadius = 1.0f;
	float mWiden = 1.0f;
	bool  mNeedUpdate = false;

	ImageWrapper                     combinedVRSTex{ nullptr };
	ImageWrapper                     combinedVRSShowTex{ nullptr };
	ID3D11NvShadingRateResourceView* combinedVRSView{ nullptr };

	D3D11VariableRateShading(ID3D11Device* device);
	~D3D11VariableRateShading() { Shutdown(); }

	bool IsEnabled();

	void UpdateTargetInformation(int displayWidth, int displayHeight, int renderWidth, int renderHeight, float leftProjX, float leftProjY, float rightProjX, float rightProjY);

	void PostOMSetRenderTargets(UINT numViews, ID3D11RenderTargetView* const* renderTargetViews, ID3D11DepthStencilView* depthStencilView);

private:
	bool nvapiLoaded = false;

	int   mRenderSizeX = 0;
	int   mRenderSizeY = 0;
	int   mDisplaySizeX = 0;
	int   mDisplaySizeY = 0;
	float proj[2][2] = { 0, 0, 0, 0 };

	ID3D11Device*                    device{ nullptr };
	ID3D11DeviceContext*             context{ nullptr };
	int                              combinedWidth = 0;
	int                              combinedHeight = 0;

	void Shutdown();

	void EnableVRS();
	void DisableVRS();

	void ApplyCombinedVRS();

	void SetupCombinedVRS();

	uint8_t DistanceToVRSLevel(float distance);
	std::vector<uint8_t> CreateCombinedFixedFoveatedVRSPattern(int width, int height, int renderWidth, int renderHeight);
	std::vector<uint8_t> CreateCombinedFixedFoveatedVRSPatternDebug(int width, int height, int renderWidth, int renderHeight);
};
