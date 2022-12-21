#pragma once
#include "DirectXMath.h"
#include <d3d11.h>

#define AutoPtr(Type, Name, OffsetSE, OffsetAE) static Type& Name = (*(Type*)RELOCATION_ID(OffsetSE, OffsetAE).address())
namespace BSGraphics
{
	struct alignas(16) ViewData
	{
		DirectX::XMVECTOR m_ViewUp;
		DirectX::XMVECTOR m_ViewRight;
		DirectX::XMVECTOR m_ViewDir;
		DirectX::XMMATRIX m_ViewMat;
		DirectX::XMMATRIX m_ProjMat;
		DirectX::XMMATRIX m_ViewProjMat;
		DirectX::XMMATRIX m_UnknownMat1;
		DirectX::XMMATRIX m_ViewProjMatrixUnjittered;
		DirectX::XMMATRIX m_PreviousViewProjMatrixUnjittered;
		DirectX::XMMATRIX m_ProjMatrixUnjittered;
		DirectX::XMMATRIX m_UnknownMat2;
		float             m_ViewPort[4];  // NiRect<float> { left = 0, right = 1, top = 1, bottom = 0 }
		RE::NiPoint2      m_ViewDepthRange;
		char              _pad0[0x8];
	};
	static_assert(sizeof(ViewData) == 0x250);

	struct CameraStateData
	{
		RE::NiCamera* pReferenceCamera;
		ViewData      CamViewData;
		RE::NiPoint3  PosAdjust;
		RE::NiPoint3  CurrentPosAdjust;
		RE::NiPoint3  PreviousPosAdjust;
		bool          UseJitter;
		char          _pad0[0x8];
	};
	static_assert(sizeof(CameraStateData) == 0x290);

	//struct State
	//{
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureProjNoiseMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureProjDiffuseMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureProjNormalMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureProjNormalDetailMap;
	//	char                               _pad0[0x1C];
	//	float                              unknown[2];
	//	float                              jitter[2];
	//	uint32_t                           uiFrameCount;
	//	bool                               bInsideFrame;
	//	bool                               bLetterbox;
	//	bool                               bUnknown1;
	//	bool                               bCompiledShaderThisFrame;
	//	bool                               bUseEarlyZ;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureBlack;  // "BSShader_DefHeightMap"
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureWhite;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureGrey;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultHeightMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultReflectionCubeMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultFaceDetailMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTexEffectMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureNormalMap;
	//	RE::NiPointer<RE::NiSourceTexture> pDefaultTextureDitherNoiseMap;
	//	RE::BSTArray<CameraStateData>      kCameraDataCacheA;
	//	float                              _pad2;                  // unknown dword
	//	float                              fHaltonSequence[2][8];  // (2, 3) Halton Sequence points
	//	float                              fDynamicResolutionCurrentWidthScale;
	//	float                              fDynamicResolutionCurrentHeightScale;
	//	float                              fDynamicResolutionPreviousWidthScale;
	//	float                              fDynamicResolutionPreviousHeightScale;
	//	float                              fDynamicResolutionWidthRatio;
	//	float                              fDynamicResolutionHeightRatio;
	//	uint16_t                           usDynamicResolutionCounter;
	//};
	//static_assert(sizeof(State) == 0x118);

	struct State
	{
		RE::NiPointer<RE::NiSourceTexture> pDefaultTextureProjNoiseMap;
		RE::NiPointer<RE::NiSourceTexture> pDefaultTextureProjDiffuseMap;
		RE::NiPointer<RE::NiSourceTexture> pDefaultTextureProjNormalMap;
		RE::NiPointer<RE::NiSourceTexture> pDefaultTextureProjNormalDetailMap;
		char                               _pad0[0x1C];
		float                              unknown[2];
		float                              jitter[2];

		//unsure whether the following haven't moved in AE, couldn't find any place where they're used
		uint32_t uiFrameCount;
		bool     bInsideFrame;
		bool     bLetterbox;
		bool     bUnknown1;
		bool     bCompiledShaderThisFrame;
		bool     bUseEarlyZ;
		// ...

		// somewhere in the middle of this struct, there are bytes in AE that are not present in SE
		// therefore use GetRuntimeData to get the rest

		struct RUNTIME_DATA
		{
			RE::NiPointer<RE::NiSourceTexture> pDefaultTextureBlack;  // "BSShader_DefHeightMap"
			RE::NiPointer<RE::NiSourceTexture> pDefaultTextureWhite;
			RE::NiPointer<RE::NiSourceTexture> pDefaultTextureGrey;
			RE::NiPointer<RE::NiSourceTexture> pDefaultHeightMap;
			RE::NiPointer<RE::NiSourceTexture> pDefaultReflectionCubeMap;
			RE::NiPointer<RE::NiSourceTexture> pDefaultFaceDetailMap;
			RE::NiPointer<RE::NiSourceTexture> pDefaultTexEffectMap;
			RE::NiPointer<RE::NiSourceTexture> pDefaultTextureNormalMap;
			RE::NiPointer<RE::NiSourceTexture> pDefaultTextureDitherNoiseMap;
			RE::BSTArray<CameraStateData>      kCameraDataCacheA;
			float                              _pad2;                  // unknown dword
			float                              fHaltonSequence[2][8];  // (2, 3) Halton Sequence points
			float                              fDynamicResolutionCurrentWidthScale;
			float                              fDynamicResolutionCurrentHeightScale;
			float                              fDynamicResolutionPreviousWidthScale;
			float                              fDynamicResolutionPreviousHeightScale;
			float                              fDynamicResolutionWidthRatio;
			float                              fDynamicResolutionHeightRatio;
			uint16_t                           usDynamicResolutionCounter;
		};
		static_assert(offsetof(RUNTIME_DATA, fDynamicResolutionCurrentWidthScale) == 0xA4);

		[[nodiscard]] RUNTIME_DATA& GetRuntimeData() noexcept
		{
			return REL::RelocateMemberIfNewer<RUNTIME_DATA>(SKSE::RUNTIME_SSE_1_6_317, this, 0x58, 0x60);
		}

		[[nodiscard]] inline const RUNTIME_DATA& GetRuntimeData() const noexcept
		{
			return REL::RelocateMemberIfNewer<RUNTIME_DATA>(SKSE::RUNTIME_SSE_1_6_317, this, 0x58, 0x60);
		}
	};

	enum RenderTargets : uint32_t
	{
		RENDER_TARGET_NONE = 0xFFFFFFFF,
		RENDER_TARGET_FRAMEBUFFER = 0,
		RENDER_TARGET_MAIN,
		RENDER_TARGET_MAIN_COPY,
		RENDER_TARGET_MAIN_ONLY_ALPHA,
		RENDER_TARGET_NORMAL_TAAMASK_SSRMASK,
		RENDER_TARGET_NORMAL_TAAMASK_SSRMASK_SWAP,
		RENDER_TARGET_NORMAL_TAAMASK_SSRMASK_DOWNSAMPLED,
		RENDER_TARGET_MOTION_VECTOR,
		RENDER_TARGET_WATER_DISPLACEMENT,
		RENDER_TARGET_WATER_DISPLACEMENT_SWAP,
		RENDER_TARGET_WATER_REFLECTIONS,
		RENDER_TARGET_WATER_FLOW,
		RENDER_TARGET_UNDERWATER_MASK,
		RENDER_TARGET_REFRACTION_NORMALS,
		RENDER_TARGET_MENUBG,
		RENDER_TARGET_PLAYER_FACEGEN_TINT,
		RENDER_TARGET_LOCAL_MAP,
		RENDER_TARGET_LOCAL_MAP_SWAP,
		RENDER_TARGET_SHADOW_MASK,
		RENDER_TARGET_GETHIT_BUFFER,
		RENDER_TARGET_GETHIT_BLURSWAP,
		RENDER_TARGET_BLURFULL_BUFFER,
		RENDER_TARGET_HDR_BLURSWAP,
		RENDER_TARGET_LDR_BLURSWAP,
		RENDER_TARGET_HDR_BLOOM,
		RENDER_TARGET_LDR_DOWNSAMPLE0,
		RENDER_TARGET_HDR_DOWNSAMPLE0,
		RENDER_TARGET_HDR_DOWNSAMPLE1,
		RENDER_TARGET_HDR_DOWNSAMPLE2,
		RENDER_TARGET_HDR_DOWNSAMPLE3,
		RENDER_TARGET_HDR_DOWNSAMPLE4,
		RENDER_TARGET_HDR_DOWNSAMPLE5,
		RENDER_TARGET_HDR_DOWNSAMPLE6,
		RENDER_TARGET_HDR_DOWNSAMPLE7,
		RENDER_TARGET_HDR_DOWNSAMPLE8,
		RENDER_TARGET_HDR_DOWNSAMPLE9,
		RENDER_TARGET_HDR_DOWNSAMPLE10,
		RENDER_TARGET_HDR_DOWNSAMPLE11,
		RENDER_TARGET_HDR_DOWNSAMPLE12,
		RENDER_TARGET_HDR_DOWNSAMPLE13,
		RENDER_TARGET_LENSFLAREVIS,
		RENDER_TARGET_IMAGESPACE_TEMP_COPY,
		RENDER_TARGET_IMAGESPACE_TEMP_COPY2,
		RENDER_TARGET_IMAGESPACE_VOLUMETRIC_LIGHTING,
		RENDER_TARGET_IMAGESPACE_VOLUMETRIC_LIGHTING_PREVIOUS,
		RENDER_TARGET_IMAGESPACE_VOLUMETRIC_LIGHTING_COPY,
		RENDER_TARGET_SAO,
		RENDER_TARGET_SAO_DOWNSCALED,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_0_ESRAM,
		RENDER_TARGET_SAO_CAMERAZ,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_0,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_1,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_2,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_3,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_4,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_5,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_6,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_7,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_8,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_9,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_10,
		RENDER_TARGET_SAO_CAMERAZ_MIP_LEVEL_11,
		RENDER_TARGET_SAO_RAWAO,
		RENDER_TARGET_SAO_RAWAO_DOWNSCALED,
		RENDER_TARGET_SAO_RAWAO_PREVIOUS,
		RENDER_TARGET_SAO_RAWAO_PREVIOUS_DOWNSCALED,
		RENDER_TARGET_SAO_TEMP_BLUR,
		RENDER_TARGET_SAO_TEMP_BLUR_DOWNSCALED,
		RENDER_TARGET_INDIRECT,
		RENDER_TARGET_INDIRECT_DOWNSCALED,
		RENDER_TARGET_RAWINDIRECT,
		RENDER_TARGET_RAWINDIRECT_DOWNSCALED,
		RENDER_TARGET_RAWINDIRECT_PREVIOUS,
		RENDER_TARGET_RAWINDIRECT_PREVIOUS_DOWNSCALED,
		RENDER_TARGET_RAWINDIRECT_SWAP,
		RENDER_TARGET_SAVE_GAME_SCREENSHOT,
		RENDER_TARGET_SCREENSHOT,
		RENDER_TARGET_VOLUMETRIC_LIGHTING_HALF_RES,
		RENDER_TARGET_VOLUMETRIC_LIGHTING_BLUR_HALF_RES,
		RENDER_TARGET_VOLUMETRIC_LIGHTING_QUARTER_RES,
		RENDER_TARGET_VOLUMETRIC_LIGHTING_BLUR_QUARTER_RES,
		RENDER_TARGET_TEMPORAL_AA_ACCUMULATION_1,
		RENDER_TARGET_TEMPORAL_AA_ACCUMULATION_2,
		RENDER_TARGET_TEMPORAL_AA_UI_ACCUMULATION_1,
		RENDER_TARGET_TEMPORAL_AA_UI_ACCUMULATION_2,
		RENDER_TARGET_TEMPORAL_AA_MASK,
		RENDER_TARGET_TEMPORAL_AA_WATER_1,
		RENDER_TARGET_TEMPORAL_AA_WATER_2,
		RENDER_TARGET_RAW_WATER,
		RENDER_TARGET_WATER_1,
		RENDER_TARGET_WATER_2,
		RENDER_TARGET_IBLENSFLARES_LIGHTS_FILTER,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_4X_4X_PING,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_4X_4X_PONG,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_16X_4Y_PING,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_16X_4Y_PONG,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_16X_4Y_BLUR,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_16X_4Y_BLUR_SWAP,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_32X_4Y_PING,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_32X_4Y_PONG,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_32X_4Y_BLUR,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_32X_4Y_BLUR_SWAP,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_16X_16X_PING,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_16X_16X_PONG,
		RENDER_TARGET_IBLENSFLARES_DOWNSAMPLE_16X_16X_SWAP,
		RENDER_TARGET_BOOK_TEXT_0,
		RENDER_TARGET_BOOK_TEXT_1,
		RENDER_TARGET_BOOK_TEXT_2,
		RENDER_TARGET_BOOK_TEXT_3,
		RENDER_TARGET_SSR,
		RENDER_TARGET_SSR_RAW,
		RENDER_TARGET_SSR_BLURRED0,
		RENDER_TARGET_SNOW_SPECALPHA,
		RENDER_TARGET_SNOW_SWAP,

		RENDER_TARGET_COUNT,
		RENDER_TARGET_FRAMEBUFFER_COUNT = 1,
	};

	enum RenderTargetsCubemaps : uint32_t
	{
		RENDER_TARGET_CUBEMAP_NONE = 0xFFFFFFFF,
		RENDER_TARGET_CUBEMAP_REFLECTIONS = 0,

		RENDER_TARGET_CUBEMAP_COUNT,
	};

	enum RenderTargets3D : uint32_t
	{
		TEXTURE3D_NONE = 0xFFFFFFFF,
		VOLUMETRIC_LIGHTING_ACCUMULATION = 0,
		VOLUMETRIC_LIGHTING_ACCUMULATION_COPY,
		VOLUMETRIC_LIGHTING_NOISE,

		TEXTURE3D_COUNT,
	};

	enum RenderTargetsDepthStencil : uint32_t
	{
		DEPTH_STENCIL_TARGET_NONE = 0xFFFFFFFF,
		DEPTH_STENCIL_TARGET_MAIN = 0,
		DEPTH_STENCIL_TARGET_MAIN_COPY,
		DEPTH_STENCIL_TARGET_SHADOWMAPS_ESRAM,
		DEPTH_STENCIL_TARGET_VOLUMETRIC_LIGHTING_SHADOWMAPS_ESRAM,
		DEPTH_STENCIL_TARGET_SHADOWMAPS,
		DEPTH_STENCIL_DECAL_OCCLUSION,
		DEPTH_STENCIL_CUBEMAP_REFLECTIONS,
		DEPTH_STENCIL_POST_ZPREPASS_COPY,
		DEPTH_STENCIL_POST_WATER_COPY,
		DEPTH_STENCIL_BOOK_TEXT,
		DEPTH_STENCIL_TARGET_PRECIPITATION_OCCLUSION_MAP,
		DEPTH_STENCIL_TARGET_FOCUS_NEO,

		DEPTH_STENCIL_COUNT,
	};

	class RendererWindow
	{
	public:
		HWND            hWnd;
		int             iWindowX;
		int             iWindowY;
		int             uiWindowWidth;
		int             uiWindowHeight;
		IDXGISwapChain* pSwapChain;
		uint32_t        SwapChainRenderTarget;
		char            _pad0[0x2C];
	};
	static_assert(sizeof(RendererWindow) == 0x50);

	struct RenderTargetData
	{
		ID3D11Texture2D*           Texture;
		ID3D11Texture2D*           TextureCopy;
		ID3D11RenderTargetView*    RTV;      // For "Texture"
		ID3D11ShaderResourceView*  SRV;      // For "Texture"
		ID3D11ShaderResourceView*  SRVCopy;  // For "TextureCopy"
		ID3D11UnorderedAccessView* UAV;      // For "Texture"
	};
	static_assert(sizeof(RenderTargetData) == 0x30);

	struct DepthStencilData
	{
		ID3D11Texture2D*          Texture;
		ID3D11DepthStencilView*   Views[8];
		ID3D11DepthStencilView*   ReadOnlyViews[8];
		ID3D11ShaderResourceView* DepthSRV;
		ID3D11ShaderResourceView* StencilSRV;
	};
	static_assert(sizeof(DepthStencilData) == 0x98);

	struct CubemapRenderTargetData
	{
		ID3D11Texture2D*          Texture;
		ID3D11RenderTargetView*   CubeSideRTV[6];
		ID3D11ShaderResourceView* SRV;
	};
	static_assert(sizeof(CubemapRenderTargetData) == 0x40);

	struct Texture3DTargetData
	{
		char _pad0[0x20];
	};
	static_assert(sizeof(Texture3DTargetData) == 0x20);

	class Renderer
	{
	public:
		char                    _pad0[0x10];
		char                    _pad1[0x22];
		bool                    bReadOnlyDepth;  // bReadOnlyStencil?
		char                    _pad2[0x15];
		ID3D11Device*           pDevice;
		ID3D11DeviceContext*    pContext;
		RendererWindow          RenderWindowA[32];
		RenderTargetData        pRenderTargets[RENDER_TARGET_COUNT];
		DepthStencilData        pDepthStencils[DEPTH_STENCIL_COUNT];
		CubemapRenderTargetData pCubemapRenderTargets[RENDER_TARGET_CUBEMAP_COUNT];
		Texture3DTargetData     pTexture3DRenderTargets[TEXTURE3D_COUNT];
		float                   ClearColor[4];
		uint8_t                 ClearStencil;
		CRITICAL_SECTION        RendererLock;

		static Renderer* QInstance()
		{
			static BSGraphics::Renderer* g_Renderer = (Renderer*)RELOCATION_ID(524907, 411393).address();  // no added ae address
			return g_Renderer;
		}
	};

}
