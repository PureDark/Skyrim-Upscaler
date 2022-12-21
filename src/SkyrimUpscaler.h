#pragma once

#include <PCH.h>
#include <PDPerfPlugin/PDPerfPlugin.h>
#include <RE/BSGraphics.h>
#include <SimpleIni.h>
#include <d3d11_4.h>
#include <SettingGUI.h>
#include <D3D11VariableRateShading.h>

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

class SkyrimUpscaler
{
public:
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
	int   mJitterPhase{ 0 };

	bool mDisableEvaluation{ false };
	bool mUseOptimalMipLodBias{ true };
	bool mEnableTransparencyMask{ false };
	bool mCancelJitter{ true };
	bool mBlurEdges{ false };
	bool mDebug{ false };
	bool mDebug2{ false };
	bool mDebug3{ false };
	bool mDebug4{ false };
	bool mDebug5{ false };
	bool mDebug6{ false };

	float mCancelScaleX{ 0.5f };
	float mCancelScaleY{ 0.5f };
	float mBlurIntensity{ 1.5f };
	float mBlendScale{ 5.0f };
	float mMotionSensitivity{ 8.0f };

	bool mDebugOverlay{ false };

	int mToggleUpscaler{ ImGuiKey_KeypadMultiply };

	ImageWrapper mTargetTex;
	ImageWrapper mTempColor;
	ImageWrapper mAccumulateTex;
	ImageWrapper mDepthBuffer;
	ImageWrapper mTempDepth;
	ImageWrapper mMotionVectors;
	ImageWrapper mOpaqueColor;
	ImageWrapper mTransparentMask;

	// For VR Fixed Foveated DLSS
	float        mFoveatedScaleX{ 0.7f };
	float        mFoveatedScaleY{ 0.57f };
	float        mFoveatedOffsetX{ 0.04f };
	float        mFoveatedOffsetY{ 0.04f };
	int          mFoveatedDisplaySizeX{ 0 };
	int          mFoveatedDisplaySizeY{ 0 };
	int          mFoveatedRenderSizeX{ 0 };
	int          mFoveatedRenderSizeY{ 0 };
	D3D11_BOX    mSrcBox[2];
	D3D11_BOX    mDstBox[2];
	FoveatedRect mSrcBoxNorm[2];
	ImageWrapper mOutColorRect[2];

	ID3D11VertexShader*    mVertexShader{ nullptr };
	ID3D11PixelShader*     mPixelShader[4]{ nullptr, nullptr, nullptr, nullptr };
	ID3D11SamplerState*    mSampler{ nullptr };
	ID3D11RasterizerState* mRasterizerState{ nullptr };
	ID3D11BlendState*      mBlendState{ nullptr };
	ID3D11Buffer*          mConstantsBuffer{ nullptr };
	CustomConstants        mCustomConstants;

	ID3D11DepthStencilState* mDepthStencilState{ nullptr };

	IDXGISwapChain*      mSwapChain{ nullptr };
	ID3D11Device*        mDevice{ nullptr };
	ID3D11DeviceContext* mContext{ nullptr };

	D3D11VariableRateShading* mVRS{ nullptr };

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
	void  Evaluate(ID3D11Resource* destTex, ID3D11DepthStencilView* dsv);
	UpscaleParams GetUpscaleParams(int id, void* color, void* motionVector, void* depth, void* mask, void* destination, int renderSizeX, int renderSizeY, float sharpness,
		float jitterOffsetX, float jitterOffsetY, int motionScaleX, int motionScaleY, bool reset, float nearPlane, float farPlane, float verticalFOV, bool execute = true);

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
	void SetupD3DBox(float offsetX, float offsetY);
	bool InFoveatedRect(float x, float y);
	void InitShader();
	void RenderTexture(int pixelShaderIndex, int numViews, ID3D11ShaderResourceView** inputSRV, ID3D11DepthStencilView* inputDSV, ID3D11RenderTargetView* target, int width, int height, int topLeftX = 0, int topLeftY = 0);
};

void InstallUpscalerHooks();
