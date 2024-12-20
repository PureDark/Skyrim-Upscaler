#pragma once
#include <dxgi1_2.h>

enum GraphicsAPI
{
	D3D11,
	D3D12,
	VULKAN
};

enum UpscaleType
{
	DLSS,
	FSR2,
	XESS,
	FSR3,
	TAA
};

enum PerfQualityLevel
{
	Performance,
	Balanced,
	Quality,
	UltraPerformance,
	UltraQuality,
	Native
};

typedef enum DLSS_Hint_Render_Preset
{
	Preset_Default,  // default behavior, may or may not change after OTA
	Preset_A,
	Preset_B,
	Preset_C,
	Preset_D,
	Preset_E,
	Preset_F,
} DLSS_Hint_Render_Preset;

typedef struct
{
	unsigned int X;
	unsigned int Y;
} Coordinates;

struct VkDevices
{
	void* vkDevice = nullptr;
	void* vkPhysicalDevice = nullptr;
	void* vkInstance = nullptr;
};

struct QueueInfo
{
	int GraphicFamilyIndex = -1;
	int GraphicQueueIndex = -1;
	int FIAsyncComputeFamilyIndex = -1;
	int FIAsyncComputeQueueIndex = -1;
	int FIPresentFamilyIndex = -1;
	int FIPresentQueueIndex = -1;
	int FIImageAcquireFamilyIndex = -1;
	int FIImageAcquireQueueIndex = -1;
};

struct InitParams
{
	int                     id;
	int                     upscaleMethod;
	int                     qualityLevel;
	int                     displaySizeX;
	int                     displaySizeY;
	int                     format;
	bool                    isContentHDR = false;
	bool                    depthInverted = false;
	bool                    YAxisInverted = false;
	bool                    motionVetorsJittered = false;
	bool                    enableSharpening = false;
	bool                    enableAutoExposure = false;
	DLSS_Hint_Render_Preset preset = Preset_Default;
	bool                    enableAsyncComputeFG = false;
	bool                    highResMotionVectors = false;
	void*                   presentCallback = nullptr;
	void*                   cmdList = nullptr;
};

struct EvaluateParams
{
	int          id;
	void*        color;
	void*        motionVector;
	void*        depth;
	void*        mask;
	void*        resolvedColor = nullptr;  // Do not use
	void*        destination;
	float        renderSizeX;
	float        renderSizeY;
	float        sharpness;
	float        jitterOffsetX;
	float        jitterOffsetY;
	float        motionScaleX;
	float        motionScaleY;
	bool         reset;
	float        nearPlane;
	float        farPlane;
	float        verticalFOV;
	bool         execute;            /* If you are calling EvaluateUpscale more than once per frame, set execute to true only at last call*/
	void*        cmdList = nullptr;  // Do not use
	Coordinates  colorBase = { 0, 0 };
	Coordinates  depthBase = { 0, 0 };
	Coordinates  motionBase = { 0, 0 };
	void*        uiTex = nullptr;
	void*        hudless = nullptr;
	unsigned int frameIndex = 0;
};

extern "C" __declspec(dllexport) bool __stdcall SetupDirectX(void* item, int graphicsAPI);

extern "C" __declspec(dllexport) bool __stdcall SetupGraphicDevice(void* item, int graphicsAPI);

extern "C" __declspec(dllexport) void* __stdcall SimpleInit(int id, int upscaleMethod, int qualityLevel, int displaySizeX, int displaySizeY, bool isContentHDR, bool depthInverted, bool YAxisInverted,
	bool motionVetorsJittered, bool enableSharpening, bool enableAutoExposure, int format = 10);

extern "C" __declspec(dllexport) void* __stdcall InitUpscaler(InitParams* params);

//0 = Performance, 1 = Balanced, 2 = Quality, 3 = UltraPerformance, 4 = DLAA
extern "C" __declspec(dllexport) void __stdcall SetDLSSPreset(int qualityLevel, DLSS_Hint_Render_Preset preset);

extern "C" __declspec(dllexport) void __stdcall SimpleEvaluate(int id, void* color, void* motionVector, void* depth, void* mask, void* destination, int renderSizeX, int renderSizeY, float sharpness,
	float jitterOffsetX, float jitterOffsetY, int motionScaleX, int motionScaleY, bool reset, float nearPlane, float farPlane, float verticalFOV, bool execute = true);

extern "C" __declspec(dllexport) void __stdcall EvaluateUpscaler(EvaluateParams* params);

extern "C" __declspec(dllexport) void __stdcall EvaluateFrameGeneration(EvaluateParams* params);

extern "C" __declspec(dllexport) void __stdcall SetMotionScaleX(int id, float motionScaleX);

extern "C" __declspec(dllexport) void __stdcall SetMotionScaleY(int id, float motionScaleX);

extern "C" __declspec(dllexport) int __stdcall GetRenderWidth(int id);

extern "C" __declspec(dllexport) int __stdcall GetRenderHeight(int id);

extern "C" __declspec(dllexport) float __stdcall GetOptimalSharpness(int id);

extern "C" __declspec(dllexport) float __stdcall GetOptimalMipmapBias(int id);

extern "C" __declspec(dllexport) void __stdcall SetDebug(bool debug = true);

extern "C" __declspec(dllexport) void __stdcall ReleaseUpscaleFeature(int id);

extern "C" __declspec(dllexport) int __stdcall GetJitterPhaseCount(int id);

extern "C" __declspec(dllexport) int __stdcall GetJitterOffset(float* outX, float* outY, int index, int phaseCount);

extern "C" __declspec(dllexport) void __stdcall InitLogDelegate(void (*Log)(char* message, int iSize));

extern "C" __declspec(dllexport) bool __stdcall IsUpscaleMethodAvailable(int upscaleMethod);

extern "C" __declspec(dllexport) char* __stdcall GetUpscaleMethodName(int upscaleMethod);

extern "C" __declspec(dllexport) bool __stdcall QueryRenderSize(int inDisplaySizeX, int inDisplaySizeY, int inQualValue, int* outRenderSizeX, int* outRenderSizeY);

extern "C" __declspec(dllexport) void* __stdcall GetSwapChainProxy(void* swapchain);

extern "C" __declspec(dllexport) HRESULT __stdcall CreateSwapChainProxy(IDXGIFactory* pFactory, IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain);

extern "C" __declspec(dllexport) HRESULT __stdcall CreateSwapChainProxyForHwnd(IDXGIFactory2* pFactory, IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain);

extern "C" __declspec(dllexport) void __stdcall SetFrameGeneration(bool enable);

extern "C" __declspec(dllexport) void __stdcall SetBackBufferFormat(int newFormat);
