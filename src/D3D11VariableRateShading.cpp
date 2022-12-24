#include "D3D11VariableRateShading.h"
#include "SkyrimUpscaler.h"
#include "DRS.h"

uint8_t D3D11VariableRateShading::DistanceToVRSLevel(float distance)
{
	if (distance < mInnerRadius) {
		return 0;
	}
	if (distance < mMiddleRadius) {
		return 1;
	}
	if (distance < mOutterRadius) {
		return 2;
	}
	if (distance < mCutoutRadius) {
		return 3;
	}
	return 4;
}

std::vector<uint8_t> D3D11VariableRateShading::CreateCombinedFixedFoveatedVRSPattern(int width, int height, int renderWidth, int renderHeight)
{
	float                leftProjX = proj[0][0];
	float                leftProjY = proj[0][1];
	float                rightProjX = proj[1][0];
	float                rightProjY = proj[1][1];
	std::vector<uint8_t> data(width * height);
	int                  divideWidth = renderWidth / 2;

	for (int y = 0; y < renderHeight; ++y) {
		for (int x = 0; x < divideWidth; ++x) {
			float fx = float(x) / renderWidth;
			float fy = float(y) / renderHeight;
			float distance = 2 * sqrtf((fx - leftProjX) * (fx - leftProjX) * 4 / mWiden + (fy - leftProjY) * (fy - leftProjY));
			float VRSLevel = DistanceToVRSLevel(distance);
			if (VRSLevel == 0) {
				float distance1 = 2 * sqrtf((fx - leftProjX) * (fx - leftProjX) * 4 / mWiden + (fy - leftProjY) * (fy - leftProjY) * 2);
				float distance2 = 2 * sqrtf((fx - leftProjX) * (fx - leftProjX) * 8 / mWiden + (fy - leftProjY) * (fy - leftProjY));
				float VRSLevel1 = DistanceToVRSLevel(distance1);
				float VRSLevel2 = DistanceToVRSLevel(distance2);
				if (VRSLevel1 != 0 && VRSLevel2 != 0)
					VRSLevel = 1;
			}
			data[y * width + x] = VRSLevel;
		}
		for (int x = divideWidth; x < renderWidth; ++x) {
			float fx = float(x) / renderWidth;
			float fy = float(y) / renderHeight;
			float distance = 2 * sqrtf((fx - rightProjX) * (fx - rightProjX) * 4 / mWiden + (fy - rightProjY) * (fy - rightProjY));
			float VRSLevel = DistanceToVRSLevel(distance);
			if (VRSLevel == 0) {
				float distance1 = 2 * sqrtf((fx - rightProjX) * (fx - rightProjX) * 4 / mWiden + (fy - rightProjY) * (fy - rightProjY) * 2);
				float distance2 = 2 * sqrtf((fx - rightProjX) * (fx - rightProjX) * 8 / mWiden + (fy - rightProjY) * (fy - rightProjY));
				float VRSLevel1 = DistanceToVRSLevel(distance1);
				float VRSLevel2 = DistanceToVRSLevel(distance2);
				if (VRSLevel1 != 0 && VRSLevel2 != 0)
					VRSLevel = 1;
			}
			data[y * width + x] = VRSLevel;
		}
		for (int x = renderWidth; x < width; ++x) {
			data[y * width + x] = 3;
		}
	}
	for (int y = renderHeight; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			data[y * width + x] = 3;
		}
	}

	return data;
}

std::vector<uint8_t> D3D11VariableRateShading::CreateCombinedFixedFoveatedVRSPatternDebug(int width, int height, int renderWidth, int renderHeight)
{
	float                leftProjX = proj[0][0];
	float                leftProjY = proj[0][1];
	float                rightProjX = proj[1][0];
	float                rightProjY = proj[1][1];
	std::vector<uint8_t> data(width * height * 4);
	int                  divideWidth = renderWidth / 2;

	for (int y = 0; y < renderHeight; ++y) {
		for (int x = 0; x < divideWidth; ++x) {
			float fx = float(x) / renderWidth;
			float fy = float(y) / renderHeight;
			float distance = 2 * sqrtf((fx - leftProjX) * (fx - leftProjX) * 4 / mWiden + (fy - leftProjY) * (fy - leftProjY));
			float VRSLevel = DistanceToVRSLevel(distance);
			if (VRSLevel == 0) {
				float distance1 = 2 * sqrtf((fx - leftProjX) * (fx - leftProjX) * 4 / mWiden + (fy - leftProjY) * (fy - leftProjY) * 2);
				float distance2 = 2 * sqrtf((fx - leftProjX) * (fx - leftProjX) * 8 / mWiden + (fy - leftProjY) * (fy - leftProjY));
				float VRSLevel1 = DistanceToVRSLevel(distance1);
				float VRSLevel2 = DistanceToVRSLevel(distance2);
				if (VRSLevel1 != 0 && VRSLevel2 != 0)
					VRSLevel = 1;
			}
			data[(y * width + x) * 4] = VRSLevel * 63;
			data[(y * width + x) * 4 + 1] = VRSLevel * 63;
			data[(y * width + x) * 4 + 2] = VRSLevel * 63;
			data[(y * width + x) * 4 + 3] = 255;
			if (SkyrimUpscaler::GetSingleton()->InFoveatedRect(fx, fy))
				data[(y * width + x) * 4 + 2] = 255;
		}
		for (int x = divideWidth; x < renderWidth; ++x) {
			float fx = float(x) / renderWidth;
			float fy = float(y) / renderHeight;
			float distance = 2 * sqrtf((fx - rightProjX) * (fx - rightProjX) * 4 / mWiden + (fy - rightProjY) * (fy - rightProjY));
			float VRSLevel = DistanceToVRSLevel(distance);
			if (VRSLevel == 0) {
				float distance1 = 2 * sqrtf((fx - rightProjX) * (fx - rightProjX) * 4 / mWiden + (fy - rightProjY) * (fy - rightProjY) * 2);
				float distance2 = 2 * sqrtf((fx - rightProjX) * (fx - rightProjX) * 8 / mWiden + (fy - rightProjY) * (fy - rightProjY));
				float VRSLevel1 = DistanceToVRSLevel(distance1);
				float VRSLevel2 = DistanceToVRSLevel(distance2);
				if (VRSLevel1 != 0 && VRSLevel2 != 0)
					VRSLevel = 1;
			}
			data[(y * width + x) * 4] = VRSLevel * 63;
			data[(y * width + x) * 4 + 1] = VRSLevel * 63;
			data[(y * width + x) * 4 + 2] = VRSLevel * 63;
			data[(y * width + x) * 4 + 3] = 255;
			if (SkyrimUpscaler::GetSingleton()->InFoveatedRect(fx, fy))
				data[(y * width + x) * 4 + 2] = 255;
		}
		for (int x = renderWidth; x < width; ++x) {
			data[(y * width + x) * 4] = 255;
			data[(y * width + x) * 4 + 1] = 255;
			data[(y * width + x) * 4 + 2] = 255;
			data[(y * width + x) * 4 + 3] = 255;
		}
		data[(y * width + renderWidth) * 4] = 0;
		data[(y * width + renderWidth) * 4 + 1] = 0;
		data[(y * width + renderWidth) * 4 + 2] = 0;
		data[(y * width + renderWidth) * 4 + 3] = 255;
	}
	for (int y = renderHeight; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			data[(y * width + x) * 4] = 255;
			data[(y * width + x) * 4 + 1] = 255;
			data[(y * width + x) * 4 + 2] = 255;
			data[(y * width + x) * 4 + 3] = 255;
			if (y == renderHeight && x <= renderWidth) {
				data[(y * width + x) * 4] = 0;
				data[(y * width + x) * 4 + 1] = 0;
				data[(y * width + x) * 4 + 2] = 0;
				data[(y * width + x) * 4 + 3] = 255;
			}
		}
	}

	return data;
}

D3D11VariableRateShading::D3D11VariableRateShading(ID3D11Device* device)
{
	logger::info("Trying to load NVAPI...");

	if (!nvapiLoaded) {
		NvAPI_Status result = NvAPI_Initialize();
		if (result != NVAPI_OK) {
			return;
		}
		nvapiLoaded = true;
	}

	NV_D3D1x_GRAPHICS_CAPS caps;
	memset(&caps, 0, sizeof(NV_D3D1x_GRAPHICS_CAPS));
	NvAPI_Status status = NvAPI_D3D1x_GetGraphicsCapabilities(device, NV_D3D1x_GRAPHICS_CAPS_VER, &caps);
	if (status != NVAPI_OK || !caps.bVariablePixelRateShadingSupported) {
		logger::info("Variable rate shading is not available.");
		return;
	}

	this->device = device;
	device->GetImmediateContext(&context);
	logger::info("Successfully initialized NVAPI; Variable Rate Shading is available.");
}

void D3D11VariableRateShading::UpdateTargetInformation(int displayWidth, int displayHeight, int renderWidth, int renderHeight, float leftProjX, float leftProjY, float rightProjX, float rightProjY)
{
	mDisplaySizeX = displayWidth;
	mDisplaySizeY = displayHeight;
	mRenderSizeX = renderWidth;
	mRenderSizeY = renderHeight;
	proj[0][0] = leftProjX;
	proj[0][1] = leftProjY;
	proj[1][0] = rightProjX;
	proj[1][1] = rightProjY;
	mNeedUpdate = true;
}

bool D3D11VariableRateShading::IsEnabled()
{
	return mEnableFixedFoveatedRendering && SkyrimUpscaler::GetSingleton()->IsEnabled() && !DRS::GetSingleton()->IsInFullscreenMenu();
}

void D3D11VariableRateShading::PostOMSetRenderTargets(UINT numViews, ID3D11RenderTargetView* const* renderTargetViews, ID3D11DepthStencilView* depthStencilView)
{
	if (!IsEnabled() || numViews == 0 || renderTargetViews == nullptr || renderTargetViews[0] == nullptr || mDisplaySizeX == 0) {
		DisableVRS();
		return;
	}

	ID3D11Resource* resource;
	renderTargetViews[0]->GetResource(&resource);
	if (resource != SkyrimUpscaler::GetSingleton()->mOpaqueColor.mImage) {
		DisableVRS();
		return;
	}
	ApplyCombinedVRS();
}

void D3D11VariableRateShading::ApplyCombinedVRS()
{
	if (!IsEnabled())
		return;

	SetupCombinedVRS();
	NvAPI_Status status = NvAPI_D3D11_RSSetShadingRateResourceView(context, combinedVRSView);
	if (status != NVAPI_OK) {
		logger::error("Error while setting shading rate resource view: {}", status);
		Shutdown();
		return;
	}

	EnableVRS();
}

void D3D11VariableRateShading::DisableVRS()
{
	if (!IsEnabled())
		return;
	NV_D3D11_VIEWPORT_SHADING_RATE_DESC vsrd[2];
	vsrd[0].enableVariablePixelShadingRate = false;
	vsrd[1].enableVariablePixelShadingRate = false;
	memset(vsrd[0].shadingRateTable, 0, sizeof(vsrd[0].shadingRateTable));
	memset(vsrd[1].shadingRateTable, 0, sizeof(vsrd[1].shadingRateTable));
	NV_D3D11_VIEWPORTS_SHADING_RATE_DESC srd;
	srd.version = NV_D3D11_VIEWPORTS_SHADING_RATE_DESC_VER;
	srd.numViewports = 2;
	srd.pViewports = vsrd;
	NvAPI_Status status = NvAPI_D3D11_RSSetViewportsPixelShadingRates(context, &srd);
	if (status != NVAPI_OK) {
		logger::error("Error while setting shading rates: {}", status);
		Shutdown();
	}
}

void D3D11VariableRateShading::Shutdown()
{
	DisableVRS();

	if (nvapiLoaded) {
		NvAPI_Unload();
	}
	nvapiLoaded = false;
	mEnableFixedFoveatedRendering = false;
	combinedVRSTex.Release();
	combinedVRSView->Release();
	device->Release();
	context->Release();
}

void D3D11VariableRateShading::EnableVRS()
{
	NV_D3D11_VIEWPORT_SHADING_RATE_DESC vsrd[2];
	for (int i = 0; i < 2; ++i) {
		vsrd[i].enableVariablePixelShadingRate = true;
		memset(vsrd[i].shadingRateTable, NV_PIXEL_X1_PER_RASTER_PIXEL, sizeof(vsrd[i].shadingRateTable));
		vsrd[i].shadingRateTable[0] = NV_PIXEL_X1_PER_RASTER_PIXEL;
		vsrd[i].shadingRateTable[1] = NV_PIXEL_X1_PER_2X1_RASTER_PIXELS;
		vsrd[i].shadingRateTable[2] = NV_PIXEL_X1_PER_2X2_RASTER_PIXELS;
		vsrd[i].shadingRateTable[3] = NV_PIXEL_X1_PER_4X4_RASTER_PIXELS;
		vsrd[i].shadingRateTable[4] = NV_PIXEL_X0_CULL_RASTER_PIXELS;
	}
	NV_D3D11_VIEWPORTS_SHADING_RATE_DESC srd;
	srd.version = NV_D3D11_VIEWPORTS_SHADING_RATE_DESC_VER;
	srd.numViewports = 2;
	srd.pViewports = vsrd;
	NvAPI_Status status = NvAPI_D3D11_RSSetViewportsPixelShadingRates(context, &srd);
	if (status != NVAPI_OK) {
		logger::error("Error while setting shading rates: {}", status);
		Shutdown();
	}
}

void D3D11VariableRateShading::SetupCombinedVRS()
{
	int   vrsWidth = mDisplaySizeX / NV_VARIABLE_PIXEL_SHADING_TILE_WIDTH;
	if (vrsWidth & 1)
		++vrsWidth;
	int vrsHeight = mDisplaySizeY / NV_VARIABLE_PIXEL_SHADING_TILE_HEIGHT;
	if (vrsHeight & 1)
		++vrsHeight;
	bool imageSizeChanged = (combinedVRSTex.mImage && vrsWidth == combinedWidth && vrsHeight == combinedHeight);
	if (!mNeedUpdate && (!IsEnabled() || imageSizeChanged)) {
		return;
	}

	
	int vrsRenderWidth = mRenderSizeX / NV_VARIABLE_PIXEL_SHADING_TILE_WIDTH;
	if (vrsRenderWidth & 1)
		++vrsRenderWidth;
	int vrsRenderHeight = mRenderSizeY / NV_VARIABLE_PIXEL_SHADING_TILE_HEIGHT;
	if (vrsRenderHeight & 1)
		++vrsRenderHeight;

	mNeedUpdate = false;

	combinedWidth = vrsWidth;
	combinedHeight = vrsHeight;

	if (imageSizeChanged || !combinedVRSTex.mImage) {
		logger::info("Creating combined VRS pattern texture of size  {}x{} for input texture size {}x{} and render size {}x{}", vrsWidth, vrsHeight, mDisplaySizeX, mDisplaySizeY, mRenderSizeX, mRenderSizeY);
		combinedVRSTex.Release();
		combinedVRSShowTex.Release();
		SAFE_RELEASE(combinedVRSView);

		D3D11_TEXTURE2D_DESC td = {};
		td.Width = vrsWidth;
		td.Height = vrsHeight;
		td.ArraySize = 1;
		td.Format = DXGI_FORMAT_R8_UINT;
		td.SampleDesc.Count = 1;
		td.SampleDesc.Quality = 0;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		td.CPUAccessFlags = 0;
		td.MiscFlags = 0;
		td.MipLevels = 1;
		auto                   data = CreateCombinedFixedFoveatedVRSPattern(vrsWidth, vrsHeight, vrsRenderWidth, vrsRenderHeight);
		D3D11_SUBRESOURCE_DATA srd;
		srd.pSysMem = data.data();
		srd.SysMemPitch = vrsWidth;
		srd.SysMemSlicePitch = 0;
		HRESULT result = device->CreateTexture2D(&td, &srd, &combinedVRSTex.mImage);
		if (FAILED(result)) {
			Shutdown();
			logger::error("Failed to create combined VRS pattern texture:  {}", result);
			return;
		}
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		auto data2 = CreateCombinedFixedFoveatedVRSPatternDebug(vrsWidth, vrsHeight, vrsRenderWidth, vrsRenderHeight);
		srd.pSysMem = data2.data();
		srd.SysMemPitch = vrsWidth * 4;
		srd.SysMemSlicePitch = 0;
		result = device->CreateTexture2D(&td, &srd, &combinedVRSShowTex.mImage);
		if (FAILED(result)) {
			Shutdown();
			logger::error("Failed to create combined VRS pattern debug texture:  {}", result);
			return;
		}

		logger::info("Creating combined shading rate resource view");
		td.Format = DXGI_FORMAT_R8_UINT;
		NV_D3D11_SHADING_RATE_RESOURCE_VIEW_DESC vd = {};
		vd.version = NV_D3D11_SHADING_RATE_RESOURCE_VIEW_DESC_VER;
		vd.Format = td.Format;
		vd.ViewDimension = NV_SRRV_DIMENSION_TEXTURE2D;
		vd.Texture2D.MipSlice = 0;
		NvAPI_Status status = NvAPI_D3D11_CreateShadingRateResourceView(device, combinedVRSTex.mImage, &vd, &combinedVRSView);
		if (status != NVAPI_OK) {
			Shutdown();
			logger::error("Failed to create combined VRS pattern view:  {}", result);
			return;
		}
	} else {
		auto data = CreateCombinedFixedFoveatedVRSPattern(vrsWidth, vrsHeight, vrsRenderWidth, vrsRenderHeight);
		context->UpdateSubresource(combinedVRSTex.mImage, 0, NULL, data.data(), 0, 0);
		data = CreateCombinedFixedFoveatedVRSPatternDebug(vrsWidth, vrsHeight, vrsRenderWidth, vrsRenderHeight);
		context->UpdateSubresource(combinedVRSShowTex.mImage, 0, NULL, data.data(), 0, 0);
	}
	
}
