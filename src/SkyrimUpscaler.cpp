#include <SkyrimUpscaler.h>
#include <PCH.h>
#include <DRS.h>
#include <hlsl/flip_vs.inc>
#include <hlsl/cancel_jitter_ps.inc>
#include <hlsl/blur_ps.inc>
#include <hlsl/blend_ps.inc>
#include <hlsl/debug_ps.inc>

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
	mUpscaleType = std::clamp(mUpscaleType, 0, 4);
	mQualityLevel = std::clamp(mQualityLevel, 0, 3);
	int mToggleHotKey = 0;
	GetSettingInt("Hotkeys", mToggleHotKey, ImGuiKey_End);
	SettingGUI::GetSingleton()->mToggleHotkey = mToggleHotKey;
	GetSettingInt("Hotkeys", mToggleUpscaler, ImGuiKey_KeypadMultiply);

	GetSettingBool("Settings", mEnableJitter, true);
	GetSettingBool("Settings", mCancelJitter, true);
	GetSettingBool("Settings", mUseTAAForPeriphery, true);

	GetSettingFloat("FixedFoveatedUpscaling", mFoveatedScaleX, 0.67f);
	GetSettingFloat("FixedFoveatedUpscaling", mFoveatedScaleY, 0.57f);
	GetSettingFloat("FixedFoveatedUpscaling", mFoveatedOffsetX, 0.04f);
	GetSettingFloat("FixedFoveatedUpscaling", mFoveatedOffsetY, 0.04f);

	bool  mEnableFixedFoveatedRendering;
	float mInnerRadius, mMiddleRadius, mOutterRadius, mCutoutRadius;
	float mWiden;
	GetSettingFloat("FixedFoveatedRendering", mEnableFixedFoveatedRendering, false);
	GetSettingFloat("FixedFoveatedRendering", mInnerRadius, 0.7f);
	GetSettingFloat("FixedFoveatedRendering", mMiddleRadius, 0.8f);
	GetSettingFloat("FixedFoveatedRendering", mOutterRadius, 0.9f);
	GetSettingFloat("FixedFoveatedRendering", mCutoutRadius, 1.2f);
	GetSettingFloat("FixedFoveatedRendering", mWiden, 1.0f);
	mVRS->mEnableFixedFoveatedRendering = mEnableFixedFoveatedRendering;
	mVRS->mInnerRadius = std::clamp(mInnerRadius, 0.0f, 1.0f);
	mVRS->mMiddleRadius = std::clamp(mMiddleRadius, 0.0f, 1.2f);
	mVRS->mOutterRadius = std::clamp(mOutterRadius, 0.0f, 1.5f);
	mVRS->mCutoutRadius = std::clamp(mCutoutRadius, 0.0f, 2.0f);
	mVRS->mWiden = std::clamp(mWiden, 0.1f, 4.0f);
	mEnableDelayCount = -1;

	UnkOuterStruct::GetSingleton()->SetTAA(SkyrimUpscaler::GetSingleton()->mUseTAAForPeriphery);

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
	SetSettingInt("Hotkeys", mToggleUpscaler);
	SetSettingBool("Settings", mEnableJitter);
	SetSettingBool("Settings", mCancelJitter);
	SetSettingBool("Settings", mUseTAAForPeriphery);
	SetSettingFloat("FixedFoveatedUpscaling", mFoveatedScaleX);
	SetSettingFloat("FixedFoveatedUpscaling", mFoveatedScaleY);
	SetSettingFloat("FixedFoveatedUpscaling", mFoveatedOffsetX);
	SetSettingFloat("FixedFoveatedUpscaling", mFoveatedOffsetY);

	bool mEnableFixedFoveatedRendering = mVRS->mEnableFixedFoveatedRendering;
	float mInnerRadius = mVRS->mInnerRadius; 
	float mMiddleRadius = mVRS->mMiddleRadius; 
	float mOutterRadius = mVRS->mOutterRadius;
	float mWiden = mVRS->mWiden; 
	SetSettingFloat("FixedFoveatedRendering", mEnableFixedFoveatedRendering);
	SetSettingFloat("FixedFoveatedRendering", mInnerRadius);
	SetSettingFloat("FixedFoveatedRendering", mMiddleRadius);
	SetSettingFloat("FixedFoveatedRendering", mOutterRadius);
	SetSettingFloat("FixedFoveatedRendering", mWiden);
	ini.SaveFile(L"Data\\SKSE\\Plugins\\SkyrimUpscaler.ini");
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
		SetEnabled(mEnableUpscaler);
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
	mVRS = new D3D11VariableRateShading(mDevice);
}

float SkyrimUpscaler::GetVerticalFOVRad()
{
	static float& fac = (*(float*)(RELOCATION_ID(513786, 388785).address()));
	const auto    base = fac;
	const auto    x = base / 1.30322540f;
	const auto    vFOV = 2 * atan(x / (float(mRenderSizeX) / mRenderSizeY));
	return vFOV;
}

UpscaleParams SkyrimUpscaler::GetUpscaleParams(int id, void* color, void* motionVector, void* depth, void* mask, void* destination, int renderSizeX, int renderSizeY, float sharpness,
	float jitterOffsetX, float jitterOffsetY, int motionScaleX, int motionScaleY, bool reset, float nearPlane, float farPlane, float verticalFOV, bool execute) 
{
	UpscaleParams params;
	params.id = id;
	params.color = color;
	params.motionVector = motionVector;
	params.depth = depth;
	params.mask = mask;
	params.destination = destination;
	params.renderSizeX = renderSizeX;
	params.renderSizeY = renderSizeY;
	params.sharpness = sharpness;
	params.jitterOffsetX = jitterOffsetX;
	params.jitterOffsetY = jitterOffsetY;
	params.motionScaleX = motionScaleX;
	params.motionScaleY = motionScaleY;
	params.reset = reset;
	params.nearPlane = nearPlane;
	params.farPlane = farPlane;
	params.verticalFOV = verticalFOV;
	params.execute = execute;
	return params;
}

void SkyrimUpscaler::Evaluate(ID3D11Resource* destTex, ID3D11DepthStencilView* dsv)
{
	if (mSwapChain != nullptr) {
		if (mTargetTex.mImage != nullptr && mDepthBuffer.mImage != nullptr && mMotionVectors.mImage != nullptr) {
			if ((IsEnabled() && !DRS::GetSingleton()->reset) && (mUpscaleType != TAA)) {
				if (!mDisableEvaluation) {
					float vFOV = GetVerticalFOVRad();
					auto          targetTex = mUseTAAForPeriphery ? mTempColor.mImage : mTargetTex.mImage;
					UpscaleParams params = GetUpscaleParams(0, targetTex, mMotionVectors.mImage, mDepthBuffer.mImage, nullptr, nullptr, mFoveatedRenderSizeX, mFoveatedRenderSizeY, mSharpness,
						mJitterOffsets[0], mJitterOffsets[1], mMotionScale[0], mMotionScale[1], false, 0.1f, 1000.0f, vFOV, mUpscaleType == DLSS);
					UpscaleParams params2 = GetUpscaleParams(1, targetTex, mMotionVectors.mImage, mDepthBuffer.mImage, nullptr, nullptr, mFoveatedRenderSizeX, mFoveatedRenderSizeY, mSharpness,
						mJitterOffsets[0], mJitterOffsets[1], mMotionScale[0], mMotionScale[1], false, 0.1f, 1000.0f, vFOV, true);
					params.colorBase = { mSrcBox[0].left, mSrcBox[0].top };
					params.depthBase = { mSrcBox[0].left, mSrcBox[0].top };
					params.motionBase = { mSrcBox[0].left, mSrcBox[0].top };
					params2.colorBase = { mSrcBox[1].left, mSrcBox[1].top };
					params2.depthBase = { mSrcBox[1].left, mSrcBox[1].top };
					params2.motionBase = { mSrcBox[1].left, mSrcBox[1].top };
					EvaluateUpscaler(&params);
					EvaluateUpscaler(&params2);
				}
				static ImageWrapper dest = { (ID3D11Texture2D*)destTex };
				mCustomConstants.jitterOffset[0] = mJitterOffsets[0] / mDisplaySizeX;
				mCustomConstants.jitterOffset[1] = mJitterOffsets[1] / mDisplaySizeY;
				mCustomConstants.dynamicResScale[0] = mRenderScale;
				mCustomConstants.dynamicResScale[1] = mRenderScale;
				mCustomConstants.screenSize[0] = mDisplaySizeX;
				mCustomConstants.screenSize[1] = mDisplaySizeY;
				mCustomConstants.motionSensitivity = mMotionSensitivity;
				mCustomConstants.blendScale = mBlendScale;
				if (mDisableEvaluation) {
					mCustomConstants.leftRect[0] = 0;
					mCustomConstants.leftRect[1] = 0;
					mCustomConstants.leftRect[2] = 0;
					mCustomConstants.leftRect[3] = 0;
					mCustomConstants.rightRect[0] = 0;
					mCustomConstants.rightRect[1] = 0;
					mCustomConstants.rightRect[2] = 0;
					mCustomConstants.rightRect[3] = 0;
				} else {
					mCustomConstants.leftRect[0] = mSrcBoxNorm[0].left;
					mCustomConstants.leftRect[1] = mSrcBoxNorm[0].top;
					mCustomConstants.leftRect[2] = mSrcBoxNorm[0].right;
					mCustomConstants.leftRect[3] = mSrcBoxNorm[0].bottom;
					mCustomConstants.rightRect[0] = mSrcBoxNorm[1].left;
					mCustomConstants.rightRect[1] = mSrcBoxNorm[1].top;
					mCustomConstants.rightRect[2] = mSrcBoxNorm[1].right;
					mCustomConstants.rightRect[3] = mSrcBoxNorm[1].bottom;
				}
				mContext->UpdateSubresource(mConstantsBuffer, 0, NULL, &mCustomConstants, 0, 0);
				mContext->PSSetConstantBuffers(0, 1, &mConstantsBuffer);
				if (mCancelJitter) {
					ID3D11ShaderResourceView* srvs[3] = { mTargetTex.GetSRV(), mAccumulateTex.GetSRV(), mMotionVectors.GetSRV() };
					RenderTexture(0, 3, srvs, dsv, dest.GetRTV(), mDisplaySizeX, mDisplaySizeY);
					mContext->CopyResource(mAccumulateTex.mImage, dest.mImage);
				}
				if (!mDisableEvaluation) {
					mContext->CopySubresourceRegion(mTempColor.mImage, 0, mDstBox[0].left, mDstBox[0].top, 0, mOutColorRect[0].mImage, 0, NULL);
					mContext->CopySubresourceRegion(mTempColor.mImage, 0, mDstBox[1].left, mDstBox[1].top, 0, mOutColorRect[1].mImage, 0, NULL);
					ID3D11ShaderResourceView* srvs[1] = { mTempColor.GetSRV() };
					RenderTexture(2, 1, srvs, dsv, dest.GetRTV(), mDisplaySizeX, mDisplaySizeY);
				}
				if (mBlurEdges) {
					mContext->CopyResource(mTempColor.mImage, dest.mImage);
					ID3D11ShaderResourceView* srvs[1] = { mTempColor.GetSRV() };
					RenderTexture(1, 1, srvs, dsv, dest.GetRTV(), mDisplaySizeX, mDisplaySizeY);
				}
				if (mDebugOverlay && mVRS->mEnableFixedFoveatedRendering) {
					ID3D11ShaderResourceView* srvs[1] = { mVRS->combinedVRSShowTex.GetSRV()};
					RenderTexture(3, 1, srvs, dsv, dest.GetRTV(), mDisplaySizeX, mDisplaySizeY);
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

	mSrcBoxNorm[0].left = ((1.0f - mFoveatedScaleX) / 2 + mFoveatedOffsetX) / 2;
	mSrcBoxNorm[0].top = (1.0f - mFoveatedScaleY) / 2 + mFoveatedOffsetY;
	mSrcBoxNorm[0].right = mSrcBoxNorm[0].left + mFoveatedScaleX / 2;
	mSrcBoxNorm[0].bottom = mSrcBoxNorm[0].top + mFoveatedScaleY;

	mSrcBoxNorm[1].left = ((1.0f - mFoveatedScaleX) / 2 - mFoveatedOffsetX) / 2 + 0.5f;
	mSrcBoxNorm[1].top = mSrcBoxNorm[0].top;
	mSrcBoxNorm[1].right = mSrcBoxNorm[1].left + mFoveatedScaleX / 2;
	mSrcBoxNorm[1].bottom = mSrcBoxNorm[0].bottom;

	float leftCenterX = (mSrcBoxNorm[0].left + mSrcBoxNorm[0].right) / 2;
	float leftCenterY = (mSrcBoxNorm[0].top + mSrcBoxNorm[0].bottom) / 2;
	float rightCenterX = (mSrcBoxNorm[1].left + mSrcBoxNorm[1].right) / 2;
	float rightCenterY = (mSrcBoxNorm[1].top + mSrcBoxNorm[1].bottom) / 2;
	if(DRS::GetSingleton()->targetScaleFactor < 1.0f)
		mVRS->UpdateTargetInformation(mDisplaySizeX, mDisplaySizeY, mRenderSizeX, mRenderSizeY, leftCenterX, leftCenterY, rightCenterX, rightCenterY);
	else
		mVRS->UpdateTargetInformation(mDisplaySizeX, mDisplaySizeY, mDisplaySizeX, mDisplaySizeY, leftCenterX, leftCenterY, rightCenterX, rightCenterY);

	renderX = (float)mDisplaySizeX / 2;
	mDstBox[0].left = (renderX - mFoveatedDisplaySizeX) / 2 + mFoveatedOffsetX * renderX;
	mDstBox[0].top = (mDisplaySizeY - mFoveatedDisplaySizeY) / 2 + mFoveatedOffsetY * mDisplaySizeY;
	mDstBox[1].left = (renderX - mFoveatedDisplaySizeX) / 2 + renderX - mFoveatedOffsetX * renderX;
	mDstBox[1].top = (mDisplaySizeY - mFoveatedDisplaySizeY) / 2 + mFoveatedOffsetY * mDisplaySizeY;
}

bool SkyrimUpscaler::InFoveatedRect(float x, float y)
{
	return (((x >= mSrcBoxNorm[0].left && y >= mSrcBoxNorm[0].top) && (x <= mSrcBoxNorm[0].right && y <= mSrcBoxNorm[0].bottom)) || ((x >= mSrcBoxNorm[1].left && y >= mSrcBoxNorm[1].top) && (x <= mSrcBoxNorm[1].right && y <= mSrcBoxNorm[1].bottom)));
}

bool SkyrimUpscaler::IsEnabled()
{
	return mEnableUpscaler;
}

void SkyrimUpscaler::DelayEnable()
{
	if (mEnableDelayCount > 0)
		mEnableDelayCount--;
	if (mEnableDelayCount == 0) {
		mEnableDelayCount = -1;
		SetEnabled(mDelayEnable);
	}
}

void SkyrimUpscaler::GetJitters(float* out_x, float* out_y)
{
	//const auto phase = GetJitterPhaseCount(0) / 2;

	mJitterIndex++;
	GetJitterOffset(out_x, out_y, mJitterIndex, mJitterPhase);
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
	float leftCenterX = (mSrcBoxNorm[0].left + mSrcBoxNorm[0].right) / 2;
	float leftCenterY = (mSrcBoxNorm[0].top + mSrcBoxNorm[0].bottom) / 2;
	float rightCenterX = (mSrcBoxNorm[1].left + mSrcBoxNorm[1].right) / 2;
	float rightCenterY = (mSrcBoxNorm[1].top + mSrcBoxNorm[1].bottom) / 2;
	mEnableUpscaler = enabled;
	if (mEnableUpscaler && mUpscaleType != TAA) {
		DRS::GetSingleton()->targetScaleFactor = mRenderScale;
		DRS::GetSingleton()->ControlResolution();
		if (mUseOptimalMipLodBias)
			mMipLodBias = (mUpscaleType == DLAA) ? 0 : GetOptimalMipmapBias(0);
		UnkOuterStruct::GetSingleton()->SetTAA(SkyrimUpscaler::GetSingleton()->mUseTAAForPeriphery);
		mVRS->UpdateTargetInformation(mDisplaySizeX, mDisplaySizeY, mRenderSizeX, mRenderSizeY, leftCenterX, leftCenterY, rightCenterX, rightCenterY);
	} else {
		DRS::GetSingleton()->targetScaleFactor = 1.0f;
		DRS::GetSingleton()->ControlResolution();
		if (mUseOptimalMipLodBias)
			mMipLodBias = 0;
		UnkOuterStruct::GetSingleton()->SetTAA(SkyrimUpscaler::GetSingleton()->mUseTAAForPeriphery);
		mVRS->UpdateTargetInformation(mDisplaySizeX, mDisplaySizeY, mDisplaySizeX, mDisplaySizeY, leftCenterX, leftCenterY, rightCenterX, rightCenterY);
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
	if (!mMotionVectors.mImage) {
		D3D11_TEXTURE2D_DESC desc;
		motion_buffer->GetDesc(&desc);
		mDisplaySizeX = desc.Width;
		mDisplaySizeY = desc.Height;
	}
	mMotionVectors.mImage = motion_buffer;
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
		if (!mAccumulateTex.mImage) {
			desc.Width = mDisplaySizeX;
			desc.Height = mDisplaySizeY;
			mDevice->CreateTexture2D(&desc, NULL, &mAccumulateTex.mImage);
		}
		if (!mTempDepth.mImage) {
			D3D11_TEXTURE2D_DESC desc2;
			mDepthBuffer.mImage->GetDesc(&desc2);
			desc2.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			mDevice->CreateTexture2D(&desc2, NULL, &mTempDepth.mImage);
		}
		mFoveatedRenderSizeX = mRenderSizeX * mFoveatedScaleX / 2;
		mFoveatedRenderSizeY = mRenderSizeY * mFoveatedScaleY;
		mMotionScale[0] = mRenderSizeX/2;
		mMotionScale[1] = mRenderSizeY;
		SetMotionScaleX(0, mMotionScale[0]);
		SetMotionScaleY(0, mMotionScale[1]);
		SetMotionScaleX(1, mMotionScale[0]);
		SetMotionScaleY(1, mMotionScale[1]);
		mJitterPhase = GetJitterPhaseCount(0);
		if (mEnableUpscaler && mUpscaleType != DLAA) {
			DRS::GetSingleton()->targetScaleFactor = mRenderScale;
			DRS::GetSingleton()->ControlResolution();
			if (mUseOptimalMipLodBias)
				mMipLodBias = GetOptimalMipmapBias(0);
		} else {
			DRS::GetSingleton()->targetScaleFactor = 1.0f;
			DRS::GetSingleton()->ControlResolution();
			if (mUseOptimalMipLodBias)
				mMipLodBias = 0;
		}
		UnkOuterStruct::GetSingleton()->SetTAA(SkyrimUpscaler::GetSingleton()->mUseTAAForPeriphery);
	} else {
		DRS::GetSingleton()->targetScaleFactor = 1.0f;
		DRS::GetSingleton()->ControlResolution();
		if (mUseOptimalMipLodBias)
			mMipLodBias = 0;
		UnkOuterStruct::GetSingleton()->SetTAA(SkyrimUpscaler::GetSingleton()->mUseTAAForPeriphery);
	}
	SetupD3DBox(mFoveatedOffsetX, mFoveatedOffsetY);
}

void SkyrimUpscaler::InitShader()
{
	mDevice->CreateVertexShader(flip_vs, sizeof(flip_vs), nullptr, &mVertexShader);
	mDevice->CreatePixelShader(cancel_jitter_ps, sizeof(cancel_jitter_ps), nullptr, &mPixelShader[0]);
	mDevice->CreatePixelShader(blur_ps, sizeof(blur_ps), nullptr, &mPixelShader[1]);
	mDevice->CreatePixelShader(blend_ps, sizeof(blend_ps), nullptr, &mPixelShader[2]);
	mDevice->CreatePixelShader(debug_ps, sizeof(debug_ps), nullptr, &mPixelShader[3]);

	D3D11_SAMPLER_DESC sd;
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
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

	
	D3D11_DEPTH_STENCIL_DESC depthDesc;
	ZeroMemory(&depthDesc, sizeof(depthDesc));

	depthDesc.DepthEnable = false;
	depthDesc.StencilEnable = true;
	depthDesc.StencilReadMask = 0xFF;
	depthDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	depthDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

	mDevice->CreateDepthStencilState(&depthDesc, &mDepthStencilState);

	mCustomConstants.jitterOffset[0] = 0;
	mCustomConstants.jitterOffset[1] = 0;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	bd.ByteWidth = sizeof(CustomConstants);
	D3D11_SUBRESOURCE_DATA init;
	ZeroMemory(&init, sizeof(D3D11_SUBRESOURCE_DATA));
	init.SysMemPitch = 0;
	init.SysMemSlicePitch = 0;
	init.pSysMem = &mCustomConstants;
	auto hr = mDevice->CreateBuffer(&bd, &init, &mConstantsBuffer);
}

void SkyrimUpscaler::RenderTexture(int pixelShaderIndex, int numViews, ID3D11ShaderResourceView** inputSRV, ID3D11DepthStencilView* inputDSV, ID3D11RenderTargetView* target, int width, int height, int topLeftX, int topLeftY)
{
	mContext->OMSetRenderTargets(1, &target, inputDSV);
	mContext->OMSetBlendState(mBlendState, nullptr, 0xffffffff);
	mContext->OMSetDepthStencilState(mDepthStencilState, 0);
	mContext->VSSetShader(mVertexShader, nullptr, 0);
	mContext->PSSetShader(mPixelShader[pixelShaderIndex], nullptr, 0);
	mContext->PSSetShaderResources(0, numViews, inputSRV);
	mContext->PSSetSamplers(0, 1, &mSampler);
	mContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	mContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	mContext->IASetInputLayout(nullptr);
	mContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	D3D11_VIEWPORT vp;
	vp.TopLeftX = topLeftX;
	vp.TopLeftY = topLeftY;
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
