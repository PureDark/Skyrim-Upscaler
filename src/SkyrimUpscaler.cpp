#include <SkyrimUpscaler.h>
#include <PCH.h>
#include <DRS.h>
#include <ReShadePlugin.h>


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
	mUpscaleType = std::clamp(mUpscaleType, 0, 3);
	mQualityLevel = std::clamp(mQualityLevel, 0, 3);
	int mToggleHotkey = ImGuiKey_End;
	GetSettingInt("Hotkeys", mToggleHotkey, ImGuiKey_End);
	SettingGUI::GetSingleton()->mToggleHotkey = mToggleHotkey;

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
	int mToggleHotkey = SettingGUI::GetSingleton()->mToggleHotkey;
	SetSettingInt("Hotkeys", mToggleHotkey);
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

void SkyrimUpscaler::SetupCameraData()
{
	//static float& g_fNear = (*(float*)(RELOCATION_ID(517032, 403540).address() + 0x40));  // 2F26FC0, 2FC1A90
	//static float& g_fFar = (*(float*)(RELOCATION_ID(517032, 403540).address() + 0x44));   // 2F26FC4, 2FC1A94
	mFOV = GetVerticalFOVRad();
	mNearPlane = 0.1f;
	mFarPlane = 1000.0f;
}

void SkyrimUpscaler::EvaluateUpscaler()
{
	static bool lastEnable = false;
	if (mSwapChain != nullptr) {
		ID3D11Texture2D* back_buffer;
		mSwapChain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
		if (back_buffer != nullptr && mDepthBuffer.mImage != nullptr && mMotionVectors.mImage != nullptr) {
			bool             enable = IsEnabled();
			bool             delayOneFrame = (lastEnable && !enable);
			bool             TAAEnabled = (mUpscaleType == TAA);
			if (((IsEnabled() && !DRS::GetSingleton()->reset) || delayOneFrame) && !TAAEnabled) {
				lastEnable = enable;
				ID3D11DeviceContext* context;
				mDevice->GetImmediateContext(&context);
				// For DLSS to work in borderless mode we must copy the backbuffer to a temporary texture
				context->CopyResource(mTempColor.mImage, back_buffer);
				int j = (mEnableJitter) ? 1 : 0;
				if (!mDisableEvaluation) {
					SimpleEvaluate(0, mTempColor.mImage, mMotionVectors.mImage, mDepthBuffer.mImage, nullptr, back_buffer, mRenderSizeX, mRenderSizeY, mSharpness, 
						mJitterOffsets[0] * j, mJitterOffsets[1] * j, mMotionScale[0], mMotionScale[1], false, mNearPlane, mFarPlane, mFOV);
					context->CopyResource(mMotionVectors.mImage, mMotionVectorsEmpty.mImage);
				}
			}
		}
	}
}

void SkyrimUpscaler::ForceEvaluateUpscaler(ID3D11Texture2D* color, ID3D11Texture2D* dest)
{
	static bool        lastEnable = false;

	if (mDepthBuffer.mImage != nullptr && mMotionVectors.mImage != nullptr) {
		ID3D11DeviceContext* context;
		mDevice->GetImmediateContext(&context);
		if (!mDisableResultCopying) {
			SimpleEvaluate(0, color, mMotionVectors.mImage, mDepthBuffer.mImage, nullptr, dest, mRenderSizeX, mRenderSizeY, mSharpness,
				mJitterOffsets[0], mJitterOffsets[1], mMotionScale[0], mMotionScale[1], false, mNearPlane, mFarPlane, mFOV);
			context->CopyResource(mMotionVectors.mImage, mMotionVectorsEmpty.mImage);
		}
	}
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

void SkyrimUpscaler::SetupDepth(ID3D11Texture2D* depth_buffer)
{
	mDepthBuffer.mImage = depth_buffer;
	if (!mTempDepthBuffer.mImage) {
		D3D11_TEXTURE2D_DESC desc;
		depth_buffer->GetDesc(&desc);
		desc.Width = mDisplaySizeX;
		desc.Height = mDisplaySizeY;
		mDevice->CreateTexture2D(&desc, NULL, &mTempDepthBuffer.mImage);
	}
}

void SkyrimUpscaler::SetupMotionVector(ID3D11Texture2D* motion_buffer)
{
	mMotionVectors.mImage = motion_buffer;
	if (!mMotionVectorsEmpty.mImage) {
		D3D11_TEXTURE2D_DESC desc;
		motion_buffer->GetDesc(&desc);
		mDevice->CreateTexture2D(&desc, NULL, &mMotionVectorsEmpty.mImage);
	}
}

void SkyrimUpscaler::PreInit()
{
	ID3D11Texture2D* back_buffer;
	mSwapChain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
	D3D11_TEXTURE2D_DESC desc;
	back_buffer->GetDesc(&desc);
	mDisplaySizeX = desc.Width;
	mDisplaySizeY = desc.Height;
}

void SkyrimUpscaler::InitUpscaler()
{
	SetDebug(mDebug);
	ID3D11Texture2D* back_buffer;
	mSwapChain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
	D3D11_TEXTURE2D_DESC desc;
	back_buffer->GetDesc(&desc);
	mDisplaySizeX = desc.Width;
	mDisplaySizeY = desc.Height;
	if (mUpscaleType != TAA) {
		int upscaleType = (mUpscaleType == DLAA) ? DLSS : mUpscaleType;
		// These two run on DX12, need to make sure the job of last frame is done before reinitializing
		if (upscaleType == FSR2 || upscaleType == XESS)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		mOutColor.mImage = (ID3D11Texture2D*)SimpleInit(0, upscaleType, mQualityLevel, mDisplaySizeX, mDisplaySizeY, false, false, false, false, mSharpening, true, desc.Format);
		if (mOutColor.mImage == nullptr) {
			SetEnabled(false);
			return;
		}
		if (!mTempColor.mImage)
			mDevice->CreateTexture2D(&desc, NULL, &mTempColor.mImage);
		if (mUpscaleType == DLAA) {
			mRenderSizeX = mDisplaySizeX;
			mRenderSizeY = mDisplaySizeY;
			mRenderScale = 1.0f;
		} else {
			mRenderSizeX = GetRenderWidth(0);
			mRenderSizeY = GetRenderHeight(0);
			mRenderScale = float(mRenderSizeX) / mDisplaySizeX;
		}
		mMotionScale[0] = mRenderSizeX;
		mMotionScale[1] = mRenderSizeY;
		SetMotionScaleX(0, mMotionScale[0]);
		SetMotionScaleY(0, mMotionScale[1]);
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
		SetupCameraData();
	} else {
		DRS::GetSingleton()->targetScaleFactor = 1.0f;
		DRS::GetSingleton()->ControlResolution();
		mMipLodBias = 0;
		UnkOuterStruct::GetSingleton()->SetTAA(mEnableUpscaler);
	}
}
