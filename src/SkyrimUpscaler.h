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

	bool mDisableResultCopying{ false };
	bool mUseOptimalMipLodBias{ true };

	ID3D11Texture2D* mTempColor{ nullptr };
	ID3D11Texture2D* mOutColor{ nullptr };
	ID3D11Texture2D* mMotionVectorsEmpty{ nullptr };
	ID3D11Texture2D* mDepthBuffer{ nullptr };
	ID3D11Texture2D* mMotionVectors{ nullptr };
	IDXGISwapChain*  mSwapChain{ nullptr };
	ID3D11Device*    mD3d11Device{ nullptr };

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
	void  EvaluateUpscaler();

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
