#pragma once

#include <PCH.h>
#include <PDPerfPlugin/PDPerfPlugin.h>
#include <RE/BSGraphics.h>
#include <SimpleIni.h>
#include <d3d11_4.h>
#include <SettingGUI.h>

struct UnkOuterStruct
{
	struct UnkInnerStruct
	{
		uint8_t unk00[0x18];  // 00
		bool    bTAA;         // 18
	};

	// members
	uint8_t         unk00[0x1F0];    // 00
	UnkInnerStruct* unkInnerStruct;  // 1F0

	static UnkOuterStruct* GetSingleton()
	{
		REL::Relocation<UnkOuterStruct*&> instance{ RELOCATION_ID(527731, 414660) };  // 31D11A0, 326B280
		return instance.get();
	}

	bool GetTAA() const
	{
		if (this == nullptr)
			return false;
		return unkInnerStruct->bTAA;
	}

	void SetTAA(bool a_enabled)
	{
		if (this == nullptr)
			return;
		unkInnerStruct->bTAA = a_enabled;
	}
};

enum UpscaleType
{
	DLSS,
	FSR2,
	XESS,
	DLAA,
	TAA
};

struct ImageWrapper
{
public:
	ID3D11Texture2D*          mImage{ nullptr };
	ID3D11RenderTargetView*   mRTV{ nullptr };
	ID3D11ShaderResourceView* mSRV{ nullptr };
	ID3D11DepthStencilView*   mDSV{ nullptr };
	ID3D11RenderTargetView* GetRTV() {
		if (mImage!= nullptr && mRTV == nullptr) {
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

struct MenuScreenData
{
	uint32_t unk00;         // 00
	float    mouseX;        // 04
	float    mouseY;        // 08
	uint32_t unk0C;         // 0C
	uint32_t unk10;         // 10
	float    screenWidth;   // 14
	float    screenHeight;  // 18
	uint32_t unk1C;         // 1C
	uint64_t unk20;         // 20
	uint64_t unk28;         // 28
};
static_assert(sizeof(MenuScreenData) == 0x30);

const float Deg2Rad = 0.0174532924f;
const float Rad2Deg = 57.29578f;

class SkyrimUpscaler
{
public:
	//Reserve the second value for VR
	bool  mEnableUpscaler = false;
	float mJitterIndex{0};
	float mJitterOffsets[2];
	float mMotionScale[2]{ 0, 0 };
	bool  mSharpening{ false };
	float mSharpness{ 0.3f };
	bool  mEnableJitter{ true };
	int   mDisplaySizeX{ 0 };
	int   mDisplaySizeY{ 0 };
	int   mRenderSizeX{ 0 };
	int   mRenderSizeY{ 0 };
	float mRenderScale{ 1.0f };
	int   mUpscaleType{ 0 };
	int   mQualityLevel{ 0 };
	float mMipLodBias{ 0 };
	float mFOV{ 70 };
	float mFarPlane{ 1000 };
	float mNearPlane{ 0.3f };

	bool mDisableEvaluation{ false };
	bool mUseOptimalMipLodBias{ true };
	bool mDebug{ false };

	ImageWrapper     mTempColor;
	ImageWrapper     mOutColor;
	ImageWrapper     mMotionVectorsEmpty;
	ImageWrapper     mDepthBuffer;
	ImageWrapper     mTempDepthBuffer;
	ImageWrapper     mMotionVectors;
	IDXGISwapChain*  mSwapChain{ nullptr };
	ID3D11Device*    mDevice{ nullptr };

	~SkyrimUpscaler() {}

	static SkyrimUpscaler* GetSingleton()
	{
		static SkyrimUpscaler handler;
		return &handler;
	}

	void SetupSwapChain(IDXGISwapChain* swapchain);

	void LoadINI();
	void SaveINI();
	void MessageHandler(SKSE::MessagingInterface::Message* a_msg);

	float GetVerticalFOVRad();
	void  SetupCameraData();
	void  EvaluateUpscaler();
	void  ForceEvaluateUpscaler(ID3D11Texture2D* color, ID3D11Texture2D* dest);

	bool IsEnabled();

	void GetJitters(float* out_x, float* out_y);
	void SetJitterOffsets(float x, float y);

	void SetMotionScale(float x, float y);
	void SetEnabled(bool enabled);
	void SetupDepth(ID3D11Texture2D* depth_buffer);
	void SetupMotionVector(ID3D11Texture2D* motion_buffer);
	void PreInit();
	void InitUpscaler();
};

void InstallUpscalerHooks();
