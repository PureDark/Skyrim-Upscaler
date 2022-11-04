#pragma once

#include <RE/BSGraphics.h>
#include <PDPerfPlugin/PDPerfPlugin.h>
#include <d3d11_4.h>
#include <PCH.h>
#include <SimpleIni.h>
#include <RE/BSGraphics.h>

class SkyrimUpscaler
{
public:
	//Reserve the second value for VR
	bool  mEnableUpscaler = false;
	float mJitterIndex[2]{ 0, 0 };
	float mJitterOffsets[2][2];
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
	bool mDisableResultCopying{ false };

	ID3D11Texture2D* mTempColor;
	ID3D11Texture2D* mOutColor;
	ID3D11Texture2D* mMotionVectorsEmpty;
	ID3D11Texture2D* mDepthBuffer;
	ID3D11Texture2D* mMotionVectors;
	IDXGISwapChain*  mSwapChain;
	ID3D11Device*    mD3d11Device;

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

	void  GetJitters(float* out_x, float* out_y);
	void  SetJitterOffsets(float x, float y);

	void  SetMotionScale(float x, float y);
	void  SetEnabled(bool enabled);
	void  SetupDepth(ID3D11Texture2D* depth_buffer);
	void  SetupMotionVector(ID3D11Texture2D* motion_buffer);
	void  InitUpscaler();

};

struct UpscalerHooks
{
	struct Main_DrawWorld_MainDraw
	{
		static void thunk(INT64 BSGraphics_Renderer, int unk)
		{
			func(BSGraphics_Renderer, unk);
			SkyrimUpscaler::GetSingleton()->EvaluateUpscaler();
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct BSShaderRenderTargets_Create_MotionVector
	{
		static void thunk(void* BSShaderRenderTargets, uint32_t TargetIndex, INT64 Properties)
		{
			func(BSShaderRenderTargets, TargetIndex, Properties);
			//if (((RenderTargetType)TargetIndex) == RenderTargetType::RENDER_TARGET_MOTION_VECTOR) {
			//	logger::info("Rendering Motion Vectors");
			//	auto renderTarget = ((BSGraphics::RenderTargetData*)BSShaderRenderTargets);
			//	SkyrimUpscaler::GetSingleton()->SetupMotionVector(renderTarget->Texture);
			//}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct BSGraphics_Renderer_Begin_UpdateJitter
	{
		static void thunk(BSGraphics::State* a_state)
		{
			func(a_state);
			if (SkyrimUpscaler::GetSingleton()->mEnableUpscaler && SkyrimUpscaler::GetSingleton()->mEnableJitter) {
				float x = 0.0f;
				float y = 0.0f;
				SkyrimUpscaler::GetSingleton()->GetJitters(&x, &y);
				float w = SkyrimUpscaler::GetSingleton()->mRenderSizeX;
				float h = SkyrimUpscaler::GetSingleton()->mRenderSizeY;
				a_state->jitter[0] = -2 * x / w;
				a_state->jitter[1] = 2 * y / h;
				SkyrimUpscaler::GetSingleton()->SetJitterOffsets(-x, -y);
			} else {
				a_state->jitter[0] = 0;
				a_state->jitter[1] = 0;
				SkyrimUpscaler::GetSingleton()->SetJitterOffsets(0, 0);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	static void Install()
	{
		// Pre-UI Hook for upscaling
		stl::write_thunk_call<Main_DrawWorld_MainDraw>(REL::RelocationID(79947, 77226).address() + REL::Relocate(0x16F, 0x2BC));  // 140ebf510
		// Setup our own jitters
		stl::write_thunk_call<BSGraphics_Renderer_Begin_UpdateJitter>(REL::RelocationID(75460, 77226).address() + REL::Relocate(0xE5, 0xE5));
		// Get depth and motion buffers
		stl::write_thunk_call<BSShaderRenderTargets_Create_MotionVector>(REL::RelocationID(100458, 107175).address() + REL::Relocate(0xADF, 0xADF));
		// Enable TAA jitters without TAA
		static REL::Relocation<uintptr_t> updateJitterHook{ REL::RelocationID(75709, 77518) };          // D7CFB0, DB96E0
		static REL::Relocation<uintptr_t> buildCameraStateDataHook{ REL::RelocationID(75711, 77520) };  // D7D130, DB9850
		REL::safe_write<uint8_t>(updateJitterHook.address() + REL::Relocate(0x11, 0x14), 1);
		REL::safe_write<uint8_t>(buildCameraStateDataHook.address() + REL::Relocate(0x1D8, 0x1D8), 1);
		logger::info("Installed upscaler hooks");
	}
};
