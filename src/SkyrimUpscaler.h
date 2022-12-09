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
		REL::Relocation<UnkOuterStruct*&> instance{ REL::VariantID(527731, 414660, 0x34234C0) };  // 31D11A0, 326B280, 34234C0
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
		if (mRTV)
			mRTV->Release();
		if (mSRV)
			mSRV->Release();
		if (mDSV)
			mDSV->Release();
		if (mImage)
			mImage->Release();
	}
};

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

	bool mDisableEvaluation{ false };
	bool mUseOptimalMipLodBias{ true };
	bool mEnableTransparentMask{ false };

	ImageWrapper         mTempColor;
	ImageWrapper         mTargetTex;
	ImageWrapper         mOutColor;
	ImageWrapper         mMotionVectorsEmpty;
	ImageWrapper         mDepthBuffer;
	ImageWrapper         mMotionVectors;
	ImageWrapper         mOpaqueColor;
	ImageWrapper         mTransparentMask;
	IDXGISwapChain*      mSwapChain{ nullptr };
	ID3D11Device*        mDevice{ nullptr };
	ID3D11DeviceContext* mContext{ nullptr };

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
	void  EvaluateUpscaler(ID3D11Texture2D* source = nullptr);

	bool IsEnabled();

	void GetJitters(float* out_x, float* out_y);
	void SetJitterOffsets(float x, float y);

	void SetupTarget(ID3D11Texture2D* target_buffer);
	void SetupDepth(ID3D11Texture2D* depth_buffer);
	void SetupMotionVector(ID3D11Texture2D* motion_buffer);
	void SetupOpaqueColor(ID3D11Texture2D* opaque_buffer);
	void SetupTransparentMask(ID3D11Texture2D* transparent_buffer);
	void SetMotionScale(float x, float y);
	void SetEnabled(bool enabled);
	void PreInit();
	void InitUpscaler();
};

void InstallUpscalerHooks();
