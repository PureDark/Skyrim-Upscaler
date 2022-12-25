#pragma once

#include <string>
#include <d3d11.h>
#include <wrl/client.h>
#include "nvapi.h"
#include <vector>
#include <Common.h>

class D3D11VariableRateShading
{
public:
	bool  mEnableFixedFoveatedRendering = false;
	float mInnerRadius = 0.7f;
	float mMiddleRadius = 0.8f;
	float mOutterRadius = 0.9f;
	float mCutoutRadius = 1.2f;
	float mWiden = 1.5f;
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

	uint8_t              DistanceToVRSLevel(float distance);
	std::vector<uint8_t> CreateCombinedFixedFoveatedVRSPattern(int width, int height, int renderWidth, int renderHeight);
	std::vector<uint8_t> CreateCombinedFixedFoveatedVRSPatternDebug(int width, int height, int renderWidth, int renderHeight);
};
