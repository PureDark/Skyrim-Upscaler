#include <SkyrimUpscaler.h>
#include <PCH.h>
#include <DRS.h>
#include <hlsl/flip.vs.inc>
#include <hlsl/flip.ps.inc>

#define GetSettingInt(a_section, a_setting, a_default) a_setting = (int)ini.GetLongValue(a_section, #a_setting, a_default);
#define SetSettingInt(a_section, a_setting) ini.SetLongValue(a_section, #a_setting, a_setting);

#define GetSettingFloat(a_section, a_setting, a_default) a_setting = (float)ini.GetDoubleValue(a_section, #a_setting, a_default);
#define SetSettingFloat(a_section, a_setting) ini.SetDoubleValue(a_section, #a_setting, a_setting);

#define GetSettingBool(a_section, a_setting, a_default) a_setting = ini.GetBoolValue(a_section, #a_setting, a_default);
#define SetSettingBool(a_section, a_setting) ini.SetBoolValue(a_section, #a_setting, a_setting);

void SkyrimUpscaler::LoadINI()
{
	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(L"Data\\SKSE\\Plugins\\SkyrimUpscaler.ini");
	GetSettingBool("Settings", mEnableUpscaler, false);
	GetSettingInt("Settings", mUpscaleType, 0);
	GetSettingInt("Settings", mQualityLevel, 0);
	GetSettingBool("Settings", mUseOptimalMipLodBias, true);
	GetSettingFloat("Settings", mMipLodBias, 0);
	GetSettingBool("Settings", mSharpening, false);
	GetSettingFloat("Settings", mSharpness, 0);
	mUpscaleType = std::clamp(mUpscaleType, 0, 3);
	mQualityLevel = std::clamp(mQualityLevel, 0, 3);
	GetSettingInt("Hotkeys", SettingGUI::GetSingleton()->mToggleHotkey, ImGuiKey_End);

	GetSettingFloat("FixedFoveatedUpscaling", mFoveatedScaleX, 0.67f);
	GetSettingFloat("FixedFoveatedUpscaling", mFoveatedScaleY, 0.57f);
	GetSettingFloat("FixedFoveatedUpscaling", mFoveatedOffsetX, 0.04f);
	GetSettingFloat("FixedFoveatedUpscaling", mFoveatedOffsetY, 0.04f);

	auto bFXAAEnabled = RE::GetINISetting("bFXAAEnabled:Display");
	if (!REL::Module::IsVR()) {
		if (bFXAAEnabled) {
			if (bFXAAEnabled->GetBool())
				logger::info("Forcing FXAA off.");
			bFXAAEnabled->data.b = false;
		}
	}
}
void SkyrimUpscaler::SaveINI()
{
	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(L"Data\\SKSE\\Plugins\\SkyrimUpscaler.ini");
	SetSettingBool("Settings", mEnableUpscaler);
	SetSettingInt("Settings", mUpscaleType);
	SetSettingInt("Settings", mQualityLevel);
	SetSettingBool("Settings", mUseOptimalMipLodBias);
	SetSettingFloat("Settings", mMipLodBias);
	SetSettingBool("Settings", mSharpening);
	SetSettingFloat("Settings", mSharpness);
	int mToggleHotkey = SettingGUI::GetSingleton()->mToggleHotkey;
	SetSettingInt("Hotkeys", mToggleHotkey);
	SetSettingFloat("FixedFoveatedUpscaling", mFoveatedScaleX);
	SetSettingFloat("FixedFoveatedUpscaling", mFoveatedScaleY);
	SetSettingFloat("FixedFoveatedUpscaling", mFoveatedOffsetX);
	SetSettingFloat("FixedFoveatedUpscaling", mFoveatedOffsetY);
}

void SkyrimUpscaler::MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	static bool inited = false;
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
	case SKSE::MessagingInterface::kNewGame:
	case SKSE::MessagingInterface::kPreLoadGame:
		if (!inited) {
			LoadINI();
			//InitUpscaler();
			inited = true;
		}
		break;

	case SKSE::MessagingInterface::kSaveGame:
	case SKSE::MessagingInterface::kDeleteGame:
		SaveINI();
		break;
	}
}

void SkyrimUpscaler::SetupSwapChain(IDXGISwapChain* swapchain)
{
	mSwapChain = swapchain;
	mSwapChain->GetDevice(IID_PPV_ARGS(&mDevice));
	mDevice->GetImmediateContext(&mContext);
	SetupDirectX(mDevice, 0);
}

float SkyrimUpscaler::GetVerticalFOVRad()
{
	static float& fac = (*(float*)(RELOCATION_ID(513786, 388785).address()));
	const auto    base = fac;
	const auto    x = base / 1.30322540f;
	const auto    vFOV = 2 * atan(x / (float(mRenderSizeX) / mRenderSizeY));
	return vFOV;
}

void SkyrimUpscaler::EvaluateUpscaler(ID3D11Resource* destTex)
{
	static float&      g_fNear = (*(float*)(RELOCATION_ID(517032, 403540).address() + 0x40));  // 2F26FC0, 2FC1A90
	static float&      g_fFar = (*(float*)(RELOCATION_ID(517032, 403540).address() + 0x44));   // 2F26FC4, 2FC1A94

	if (mSwapChain != nullptr) {
		if (mTargetTex.mImage != nullptr && mDepthBuffer.mImage != nullptr && mMotionVectors.mImage != nullptr) {
			if ((IsEnabled() && !DRS::GetSingleton()->reset) && (mUpscaleType != TAA)) {
				// Apply DLSS only to a smaller rect to lower the cost
				mContext->CopySubresourceRegion(mTempColorRect[0].mImage, 0, 0, 0, 0, mTargetTex.mImage, 0, &mSrcBox[0]);
				mContext->CopySubresourceRegion(mDepthRect[0].mImage, 0, 0, 0, 0, mDepthBuffer.mImage, 0, &mSrcBox[0]);
				mContext->CopySubresourceRegion(mMotionVectorRect[0].mImage, 0, 0, 0, 0, mMotionVectors.mImage, 0, &mSrcBox[0]);
				mContext->CopySubresourceRegion(mTempColorRect[1].mImage, 0, 0, 0, 0, mTargetTex.mImage, 0, &mSrcBox[1]);
				mContext->CopySubresourceRegion(mDepthRect[1].mImage, 0, 0, 0, 0, mDepthBuffer.mImage, 0, &mSrcBox[1]);
				mContext->CopySubresourceRegion(mMotionVectorRect[1].mImage, 0, 0, 0, 0, mMotionVectors.mImage, 0, &mSrcBox[1]);
				int j = (mEnableJitter) ? 1 : 0;
				if (!mDisableResultCopying) {
					float vFOV = GetVerticalFOVRad();
					SimpleEvaluate(0, mTempColorRect[0].mImage, mMotionVectorRect[0].mImage, mDepthRect[0].mImage, nullptr, nullptr, mFoveatedRenderSizeX, mFoveatedRenderSizeY, mSharpness, 
						mJitterOffsets[0] * j, mJitterOffsets[1] * j, mMotionScale[0], mMotionScale[1], false, g_fNear / 100, g_fFar / 100, vFOV);
					SimpleEvaluate(1, mTempColorRect[1].mImage, mMotionVectorRect[1].mImage, mDepthRect[1].mImage, nullptr, nullptr, mFoveatedRenderSizeX, mFoveatedRenderSizeY, mSharpness,
						mJitterOffsets[0] * j, mJitterOffsets[1] * j, mMotionScale[0], mMotionScale[1], false, g_fNear / 100, g_fFar / 100, vFOV);
					static ImageWrapper dest = { (ID3D11Texture2D*)destTex };
					mContext->CopyResource(mTempColor.mImage, dest.mImage);
					float color[4] = { 0, 0, 0, 0 };
					mContext->ClearRenderTargetView(dest.GetRTV(), color);
					mJitterConstants.jitterOffset[0] = mJitterOffsets[0] / mDisplaySizeX;
					mJitterConstants.jitterOffset[1] = mJitterOffsets[1] / mDisplaySizeY;
					mContext->UpdateSubresource(mConstantsBuffer, 0, NULL, &mJitterConstants, 0, 0);
					mContext->PSSetConstantBuffers(0, 1, &mConstantsBuffer);
					RenderTexture(mTempColor.GetSRV(), dest.GetRTV(), mDisplaySizeX, mDisplaySizeY);
					mContext->CopySubresourceRegion(destTex, 0, mDstBox[0].left, mDstBox[0].top, 0, mOutColorRect[0].mImage, 0, &mOutBox);
					mContext->CopySubresourceRegion(destTex, 0, mDstBox[1].left, mDstBox[1].top, 0, mOutColorRect[1].mImage, 0, &mOutBox);
				}
			}
		}
	}
}


void SkyrimUpscaler::SetupD3DBox(float offsetX, float offsetY)
{
	mFoveatedOffsetX = offsetX;
	mFoveatedOffsetY = offsetY;
	float renderX = (float)mRenderSizeX / 2;
	mSrcBox[0].left = (renderX - mFoveatedRenderSizeX) / 2 + mFoveatedOffsetX * renderX;
	mSrcBox[0].top = (mRenderSizeY - mFoveatedRenderSizeY) / 2 + mFoveatedOffsetY * mRenderSizeY;
	mSrcBox[0].right = mSrcBox[0].left + mFoveatedRenderSizeX;
	mSrcBox[0].bottom = mSrcBox[0].top + mFoveatedRenderSizeY;
	mSrcBox[0].front = 0;
	mSrcBox[0].back = 1;
	mSrcBox[1].left = (renderX - mFoveatedRenderSizeX) / 2 + renderX - mFoveatedOffsetX * renderX;
	mSrcBox[1].top = (mRenderSizeY - mFoveatedRenderSizeY) / 2 + mFoveatedOffsetY * mRenderSizeY;
	mSrcBox[1].right = mSrcBox[0].left + mFoveatedRenderSizeX;
	mSrcBox[1].bottom = mSrcBox[0].top + mFoveatedRenderSizeY;
	mSrcBox[1].front = 0;
	mSrcBox[1].back = 1;

	renderX = (float)mDisplaySizeX / 2;
	mDstBox[0].left = (renderX - mFoveatedDisplaySizeX) / 2 + mFoveatedOffsetX * renderX;
	mDstBox[0].top = (mDisplaySizeY - mFoveatedDisplaySizeY) / 2 + mFoveatedOffsetY * mDisplaySizeY;
	mDstBox[1].left = (renderX - mFoveatedDisplaySizeX) / 2 + renderX - mFoveatedOffsetX * renderX;
	mDstBox[1].top = (mDisplaySizeY - mFoveatedDisplaySizeY) / 2 + mFoveatedOffsetY * mDisplaySizeY;

	mOutBox.left = 0;
	mOutBox.top = 0;
	mOutBox.right = mFoveatedDisplaySizeX;
	mOutBox.bottom = mFoveatedDisplaySizeY;
	mOutBox.front = 0;
	mOutBox.back = 1;
}

bool SkyrimUpscaler::IsEnabled()
{
	return mEnableUpscaler;
}


void SkyrimUpscaler::GetJitters(float* out_x, float* out_y)
{
	const auto phase = GetJitterPhaseCount(0);

	mJitterIndex++;
	GetJitterOffset(out_x, out_y, mJitterIndex, phase);
}

void SkyrimUpscaler::SetJitterOffsets(float x, float y)
{
	mJitterOffsets[0] = x;
	mJitterOffsets[1] = y;
}

void SkyrimUpscaler::SetMotionScale(float x, float y)
{
	mMotionScale[0] = x;
	mMotionScale[1] = y;
	SetMotionScaleX(0, x);
	SetMotionScaleX(0, y);
}

void SkyrimUpscaler::SetEnabled(bool enabled)
{
	mEnableUpscaler = enabled;
	if (mEnableUpscaler && mUpscaleType != TAA) {
		DRS::GetSingleton()->targetScaleFactor = mRenderScale;
		DRS::GetSingleton()->ControlResolution();
		mMipLodBias = (mUpscaleType==DLAA)?0:GetOptimalMipmapBias(0);
		UnkOuterStruct::GetSingleton()->SetTAA(false);
	} else {
		DRS::GetSingleton()->targetScaleFactor = 1.0f;
		DRS::GetSingleton()->ControlResolution();
		mMipLodBias = 0;
		UnkOuterStruct::GetSingleton()->SetTAA(mEnableUpscaler && mUpscaleType == TAA);
	}
}

void SkyrimUpscaler::SetupTarget(ID3D11Texture2D* target_buffer)
{
	mTargetTex.mImage = target_buffer;
}

void SkyrimUpscaler::SetupDepth(ID3D11Texture2D* depth_buffer)
{
	mDepthBuffer.mImage = depth_buffer;
}

void SkyrimUpscaler::SetupOpaqueColor(ID3D11Texture2D* opaque_buffer)
{
	mOpaqueColor.mImage = opaque_buffer;
}

void SkyrimUpscaler::SetupTransparentMask(ID3D11Texture2D* transparent_buffer)
{
	mTransparentMask.mImage = transparent_buffer;
}

void SkyrimUpscaler::SetupMotionVector(ID3D11Texture2D* motion_buffer)
{
	mMotionVectors.mImage = motion_buffer;
	if (!mMotionVectorRect[0].mImage || !mMotionVectorRect[1].mImage) {
		D3D11_TEXTURE2D_DESC desc;
		motion_buffer->GetDesc(&desc);
		mDisplaySizeX = desc.Width;
		mDisplaySizeY = desc.Height;
	}
}

void SkyrimUpscaler::PreInit()
{
	LoadINI();
	InitShader();
}

void SkyrimUpscaler::InitUpscaler()
{
	D3D11_TEXTURE2D_DESC desc;
	ID3D11Texture2D*     back_buffer;
	mSwapChain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
	back_buffer->GetDesc(&desc);
	if (mUpscaleType != TAA) {
		int upscaleType = (mUpscaleType == DLAA) ? DLSS : mUpscaleType;
		// Need to make sure the job of last frame is done before reinitializing
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		mFoveatedDisplaySizeX = mDisplaySizeX * mFoveatedScaleX / 2;
		mFoveatedDisplaySizeY = mDisplaySizeY * mFoveatedScaleY;
		// Init once to get the render size for the whole texture
		mOutColorRect[0].mImage = (ID3D11Texture2D*)SimpleInit(0, upscaleType, mQualityLevel, mDisplaySizeX, mDisplaySizeY, false, false, false, false, mSharpening, true, desc.Format);
		if (mOutColorRect[0].mImage == nullptr) {
			SetEnabled(false);
			return;
		}
		if (mUpscaleType == DLAA) {
			mRenderSizeX = mDisplaySizeX;
			mRenderSizeY = mDisplaySizeY;
			mRenderScale = 1.0f;
		} else {
			mRenderSizeX = GetRenderWidth(0);
			mRenderSizeY = GetRenderHeight(0);
			mRenderScale = float(mRenderSizeX) / mDisplaySizeX;
		}
		mOutColorRect[0].mImage = (ID3D11Texture2D*)SimpleInit(0, upscaleType, mQualityLevel, mFoveatedDisplaySizeX, mFoveatedDisplaySizeY, false, false, false, false, mSharpening, true, desc.Format);
		mOutColorRect[1].mImage = (ID3D11Texture2D*)SimpleInit(1, upscaleType, mQualityLevel, mFoveatedDisplaySizeX, mFoveatedDisplaySizeY, false, false, false, false, mSharpening, true, desc.Format);
		if (mOutColorRect[0].mImage == nullptr || mOutColorRect[1].mImage == nullptr) {
			SetEnabled(false);
			return;
		}
		if (!mTempColor.mImage) {
			desc.Width = mDisplaySizeX;
			desc.Height = mDisplaySizeY;
			mDevice->CreateTexture2D(&desc, NULL, &mTempColor.mImage);
		}
		ReleaseFoveatedResources();
		mFoveatedRenderSizeX = mRenderSizeX * mFoveatedScaleX / 2;
		mFoveatedRenderSizeY = mRenderSizeY * mFoveatedScaleY;
		SetupD3DBox(mFoveatedOffsetX, mFoveatedOffsetY);
		if (!mTempColorRect[0].mImage || !mTempColorRect[1].mImage) {
			desc.Width = mFoveatedRenderSizeX;
			desc.Height = mFoveatedRenderSizeY;
			mDevice->CreateTexture2D(&desc, NULL, &mTempColorRect[0].mImage);
			mDevice->CreateTexture2D(&desc, NULL, &mTempColorRect[1].mImage);
		}
		if (!mDepthRect[0].mImage || !mDepthRect[1].mImage) {
			D3D11_TEXTURE2D_DESC desc2;
			mDepthBuffer.mImage->GetDesc(&desc2);
			desc2.Width = mFoveatedRenderSizeX;
			desc2.Height = mFoveatedRenderSizeY;
			mDevice->CreateTexture2D(&desc2, NULL, &mDepthRect[0].mImage);
			mDevice->CreateTexture2D(&desc2, NULL, &mDepthRect[1].mImage);
		}
		if (!mMotionVectorRect[0].mImage || !mMotionVectorRect[1].mImage) {
			D3D11_TEXTURE2D_DESC desc2;
			mMotionVectors.mImage->GetDesc(&desc2);
			desc2.Width = mFoveatedRenderSizeX;
			desc2.Height = mFoveatedRenderSizeY;
			mDevice->CreateTexture2D(&desc2, NULL, &mMotionVectorRect[0].mImage);
			mDevice->CreateTexture2D(&desc2, NULL, &mMotionVectorRect[1].mImage);
		}
		mMotionScale[0] = mRenderSizeX/2;
		mMotionScale[1] = mRenderSizeY;
		SetMotionScaleX(0, mMotionScale[0]);
		SetMotionScaleY(0, mMotionScale[1]);
		SetMotionScaleX(1, mMotionScale[0]);
		SetMotionScaleY(1, mMotionScale[1]);
		if (mEnableUpscaler && mUpscaleType != DLAA) {
			DRS::GetSingleton()->targetScaleFactor = mRenderScale;
			DRS::GetSingleton()->ControlResolution();	
			mMipLodBias = GetOptimalMipmapBias(0);
		} else {
			DRS::GetSingleton()->targetScaleFactor = 1.0f;
			DRS::GetSingleton()->ControlResolution();
			mMipLodBias = 0;
		}
		UnkOuterStruct::GetSingleton()->SetTAA(false);
	} else {
		DRS::GetSingleton()->targetScaleFactor = 1.0f;
		DRS::GetSingleton()->ControlResolution();
		mMipLodBias = 0;
		UnkOuterStruct::GetSingleton()->SetTAA(mEnableUpscaler);
	}
}

void SkyrimUpscaler::ReleaseFoveatedResources()
{
	mTempColorRect[0].Release();
	mTempColorRect[1].Release();
	mDepthRect[0].Release();
	mDepthRect[1].Release();
	mMotionVectorRect[0].Release();
	mMotionVectorRect[1].Release();
}

void SkyrimUpscaler::InitShader()
{
	mDevice->CreateVertexShader(VS_Flip, sizeof(VS_Flip), nullptr, &mVertexShader);
	mDevice->CreatePixelShader(PS_Flip, sizeof(PS_Flip), nullptr, &mPixelShader);

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

	mJitterConstants.jitterOffset[0] = 0;
	mJitterConstants.jitterOffset[1] = 0;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	bd.ByteWidth = 16;
	D3D11_SUBRESOURCE_DATA init;
	ZeroMemory(&init, sizeof(D3D11_SUBRESOURCE_DATA));
	init.SysMemPitch = 0;
	init.SysMemSlicePitch = 0;
	init.pSysMem = &mJitterConstants;
	auto hr = mDevice->CreateBuffer(&bd, &init, &mConstantsBuffer);
}

void SkyrimUpscaler::RenderTexture(ID3D11ShaderResourceView* sourceTexture, ID3D11RenderTargetView* target, int width, int height)
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
