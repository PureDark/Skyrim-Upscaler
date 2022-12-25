#include "SettingGUI.h"
#include <SkyrimUpscaler.h>

LRESULT WndProcHook::thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto& io = ImGui::GetIO();
	if (uMsg == WM_KILLFOCUS) {
		io.ClearInputCharacters();
		io.ClearInputKeys();
	}
	return func(hWnd, uMsg, wParam, lParam);
}
static inline const char* format_to_string(DXGI_FORMAT format)
{
	switch (format) {
	case DXGI_FORMAT_R16G16_TYPELESS:
		return "R16G16 Typeless";
	case DXGI_FORMAT_R16G16_UINT:
		return "R16G16 UInt";
	case DXGI_FORMAT_R16G16_SINT:
		return "R16G16 SInt";
	case DXGI_FORMAT_R16G16_FLOAT:
		return "R16G16 Float";
	case DXGI_FORMAT_R16G16_UNORM:
		return "R16G16 UNorm";
	case DXGI_FORMAT_R16G16_SNORM:
		return "R16G16 SNorm";
	default:
		return "     ";
	}
}

void ProcessEvent(ImGuiKey key, ImGuiKey key2);

void SettingGUI::InitIMGUI(IDXGISwapChain* swapchain, ID3D11Device* device, ID3D11DeviceContext* context)
{
	mSwapChain = swapchain;
	mDevice = device;
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	auto&    imgui_io = ImGui::GetIO();

	imgui_io.ConfigFlags = ImGuiConfigFlags_NavEnableKeyboard;
	imgui_io.BackendFlags = ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_RendererHasVtxOffset;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	DXGI_SWAP_CHAIN_DESC desc;
	swapchain->GetDesc(&desc);

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(desc.OutputWindow);
	ImGui_ImplDX11_Init(device, context);

	WndProcHook::func = reinterpret_cast<WNDPROC>(
		SetWindowLongPtr(
			desc.OutputWindow,
			GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(WndProcHook::thunk)));
	if (!WndProcHook::func)
		logger::error("SetWindowLongPtrA failed!");
	else
		logger::info("SettingGUI::InitIMGUI Success!");
}

ImVec2 GetTextureDimensions(ID3D11ShaderResourceView* view)
{
	ID3D11Resource* res = nullptr;
	view->GetResource(&res);

	ID3D11Texture2D* texture2d = nullptr;
	HRESULT          hr = res->QueryInterface(&texture2d);

	ImVec2 dim(512, 512);
	if (SUCCEEDED(hr)) {
		D3D11_TEXTURE2D_DESC desc;
		texture2d->GetDesc(&desc);
		dim.x = static_cast<float>(desc.Width);
		dim.y = static_cast<float>(desc.Height);
	}
	float biggest = max(dim.x, dim.y);
	if (biggest > 512) {
		dim.x *= 512 / biggest;
		dim.y *= 512 / biggest;
	}
	return dim;
}

void SettingGUI::OnRender()
{
	if (REL::Module::IsVR()) {
		ProcessEvent(mToggleHotkey, SkyrimUpscaler::GetSingleton()->mToggleUpscaler);
	}

	if (ImGui::IsKeyReleased(mToggleHotkey))
		toggle();
	if (ImGui::IsKeyReleased(SkyrimUpscaler::GetSingleton()->mToggleUpscaler))
		SkyrimUpscaler::GetSingleton()->SetEnabled(!SkyrimUpscaler::GetSingleton()->mEnableUpscaler);

	auto& io = ImGui::GetIO();
	io.MouseDrawCursor = mShowGUI;

	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (!REL::Module::IsVR()) {
		static bool lastShowGUI = false;
		if (mShowGUI != lastShowGUI) {
			auto controlMap = RE::ControlMap::GetSingleton();
			if (controlMap)
				controlMap->ignoreKeyboardMouse = mShowGUI;
			lastShowGUI = mShowGUI;
		}
	}

	static int initCounter = -1;
	if (initCounter > 0) {
		initCounter--;
	}
	if (initCounter == 0) {
		SkyrimUpscaler::GetSingleton()->InitUpscaler();
		initCounter = -1;
	}

	if (mShowGUI) {
		// Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Begin("Skyrim Upscaler Settings", &mShowGUI, ImGuiWindowFlags_NoCollapse);
		//ImGui::SetWindowSize(ImVec2(576, 340), 0.9f);
		if (ImGui::Checkbox("Enable", &SkyrimUpscaler::GetSingleton()->mEnableUpscaler)) {
			SkyrimUpscaler::GetSingleton()->SetEnabled(SkyrimUpscaler::GetSingleton()->mEnableUpscaler);
		}
		ImGui::Checkbox("Disable Evaluation", &SkyrimUpscaler::GetSingleton()->mDisableEvaluation);
		//ImGui::Checkbox("Cancel Jitter", &SkyrimUpscaler::GetSingleton()->mCancelJitter);
		if (ImGui::Checkbox("Use TAA For Periphery", &SkyrimUpscaler::GetSingleton()->mUseTAAForPeriphery)) {
			UnkOuterStruct::GetSingleton()->SetTAA(SkyrimUpscaler::GetSingleton()->mUseTAAForPeriphery);
		}
		//ImGui::Checkbox("Debug", &SkyrimUpscaler::GetSingleton()->mDebug);
		//ImGui::Checkbox("Debug2", &SkyrimUpscaler::GetSingleton()->mDebug2);
		//ImGui::Checkbox("Debug3", &SkyrimUpscaler::GetSingleton()->mDebug3);
		//ImGui::Checkbox("Debug4", &SkyrimUpscaler::GetSingleton()->mDebug4);
		//ImGui::Checkbox("Debug5", &SkyrimUpscaler::GetSingleton()->mDebug5);
		//ImGui::Checkbox("Debug6", &SkyrimUpscaler::GetSingleton()->mDebug6);
		//ImGui::Checkbox("Blur Edges", &SkyrimUpscaler::GetSingleton()->mBlurEdges);
		//ImGui::BeginDisabled(!SkyrimUpscaler::GetSingleton()->mBlurEdges);
		//ImGui::DragFloat("Blur Intensity", &SkyrimUpscaler::GetSingleton()->mBlurIntensity, 0.1f, 0.0f, 10.0f);
		//ImGui::EndDisabled();

		ImGui::DragFloat("Motion Sensitivity", &SkyrimUpscaler::GetSingleton()->mMotionSensitivity, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat("Blend Scale", &SkyrimUpscaler::GetSingleton()->mBlendScale, 0.1f, 0.0f, 10.0f);
		ImGui::Checkbox("Jitter", &SkyrimUpscaler::GetSingleton()->mEnableJitter);
		if (ImGui::ArrowButton("mJitterPhase-", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mJitterPhase--;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mJitterPhase+", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mJitterPhase++;
		}
		ImGui::SameLine(0, 4.0f);
		ImGui::DragInt("Jitter Phase Count", &SkyrimUpscaler::GetSingleton()->mJitterPhase, 1, 0, 128);
		//ImGui::Checkbox("Enable Transparency Mask", &SkyrimUpscaler::GetSingleton()->mEnableTransparencyMask);
		if (ImGui::Checkbox("Sharpness", &SkyrimUpscaler::GetSingleton()->mSharpening)) {
			SkyrimUpscaler::GetSingleton()->InitUpscaler();
		}
		if (ImGui::ArrowButton("mSharpness-", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mSharpness -= 0.01f;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mSharpness+", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mSharpness += 0.01f;
		}
		ImGui::SameLine(0, 4.0f);
		ImGui::DragFloat("Sharpness Amount", &SkyrimUpscaler::GetSingleton()->mSharpness, 0.01f, 0.0f, 5.0f);

		if (ImGui::Checkbox("Use Optimal Mip Lod Bias", &SkyrimUpscaler::GetSingleton()->mUseOptimalMipLodBias)) {
			if (SkyrimUpscaler::GetSingleton()->mUseOptimalMipLodBias) {
				if (SkyrimUpscaler::GetSingleton()->mUpscaleType < DLAA)
					SkyrimUpscaler::GetSingleton()->mMipLodBias = GetOptimalMipmapBias(0);
				else
					SkyrimUpscaler::GetSingleton()->mMipLodBias = 0;
			}
		}
		ImGui::BeginDisabled(SkyrimUpscaler::GetSingleton()->mUseOptimalMipLodBias);
		ImGui::DragFloat("Mip Lod Bias", &SkyrimUpscaler::GetSingleton()->mMipLodBias, 0.1f, -3.0f, 3.0f);
		if (ImGui::Button(" -2 ")) {
			SkyrimUpscaler::GetSingleton()->mMipLodBias = -2;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::Button(" -1.5 ")) {
			SkyrimUpscaler::GetSingleton()->mMipLodBias = -1.5f;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::Button(" -1 ")) {
			SkyrimUpscaler::GetSingleton()->mMipLodBias = -1;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::Button(" -0.5 ")) {
			SkyrimUpscaler::GetSingleton()->mMipLodBias = -0.5f;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::Button(" 0 ")) {
			SkyrimUpscaler::GetSingleton()->mMipLodBias = 0;
		}
		ImGui::EndDisabled();

		std::vector<const char*> imgui_combo_names{};
		imgui_combo_names.push_back("DLSS");
		imgui_combo_names.push_back("FSR2");
		imgui_combo_names.push_back("XeSS");
		imgui_combo_names.push_back("DLAA");
		//Not making it complicated
		//imgui_combo_names.push_back("TAA");

		if (ImGui::Combo("Upscale Type", (int*)&SkyrimUpscaler::GetSingleton()->mUpscaleType, imgui_combo_names.data(), imgui_combo_names.size())) {
			if (SkyrimUpscaler::GetSingleton()->mUpscaleType < 0 || SkyrimUpscaler::GetSingleton()->mUpscaleType >= imgui_combo_names.size()) {
				SkyrimUpscaler::GetSingleton()->mUpscaleType = 0;
			}

			SkyrimUpscaler::GetSingleton()->InitUpscaler();
		}
		const auto qualities = (SkyrimUpscaler::GetSingleton()->mUpscaleType == XESS) ? "Performance\0Balanced\0Quality\0UltraQuality\0" : "Performance\0Balanced\0Quality\0UltraPerformance\0";

		ImGui::BeginDisabled(SkyrimUpscaler::GetSingleton()->mUpscaleType == TAA);
		if (ImGui::Combo("Quality Level", (int*)&SkyrimUpscaler::GetSingleton()->mQualityLevel, qualities)) {
			SkyrimUpscaler::GetSingleton()->InitUpscaler();
		}

		const auto w = (float)GetRenderWidth(0);
		const auto h = (float)GetRenderHeight(0);

		if (ImGui::DragFloat("MotionScale X", &SkyrimUpscaler::GetSingleton()->mMotionScale[0], 0.01f, -w, w) ||
			ImGui::DragFloat("MotionScale Y", &SkyrimUpscaler::GetSingleton()->mMotionScale[1], 0.01f, -h, h)) {
			SkyrimUpscaler::GetSingleton()->SetMotionScale(SkyrimUpscaler::GetSingleton()->mMotionScale[0], SkyrimUpscaler::GetSingleton()->mMotionScale[1]);
		}
		ImGui::EndDisabled();

		/*ImGui::Spacing();
		if (ImGui::ArrowButton("mCancelScaleX-", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mCancelScaleX -= 0.01f;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mCancelScaleX+", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mCancelScaleX += 0.01f;
		}
		ImGui::SameLine(0, 4.0f);
		ImGui::DragFloat("Cancel Scale X", &SkyrimUpscaler::GetSingleton()->mCancelScaleX, 0.001f, 0.0f, 3.0f);
		ImGui::Spacing();
		if (ImGui::ArrowButton("mCancelScaleY-", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mCancelScaleY -= 0.01f;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mCancelScaleY+", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mCancelScaleY += 0.01f;
		}
		ImGui::SameLine(0, 4.0f);
		ImGui::DragFloat("Cancel Scale Y", &SkyrimUpscaler::GetSingleton()->mCancelScaleY, 0.001f, 0.0f, 3.0f);*/

		ImGui::Spacing();
		if (ImGui::ArrowButton("mFoveatedScaleX-", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mFoveatedScaleX -= 0.01f;
			initCounter = 25;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mFoveatedScaleX+", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mFoveatedScaleX += 0.01f;
			initCounter = 45;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Foveated Scale X", &SkyrimUpscaler::GetSingleton()->mFoveatedScaleX, 0.01f, 0.3f, 1.0f)) {
			initCounter = 45;
		}
		if (ImGui::ArrowButton("mFoveatedScaleY-", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mFoveatedScaleY -= 0.01f;
			initCounter = 45;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mFoveatedScaleY+", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mFoveatedScaleY += 0.01f;
			initCounter = 45;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Foveated Scale Y", &SkyrimUpscaler::GetSingleton()->mFoveatedScaleY, 0.01f, 0.3f, 1.0f)) {
			initCounter = 45;
		}
		if (ImGui::ArrowButton("mFoveatedOffsetX-", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mFoveatedOffsetX -= 0.01f;
			SkyrimUpscaler::GetSingleton()->SetupD3DBox(SkyrimUpscaler::GetSingleton()->mFoveatedOffsetX, SkyrimUpscaler::GetSingleton()->mFoveatedOffsetY);
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mFoveatedOffsetX+", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mFoveatedOffsetX += 0.01f;
			SkyrimUpscaler::GetSingleton()->SetupD3DBox(SkyrimUpscaler::GetSingleton()->mFoveatedOffsetX, SkyrimUpscaler::GetSingleton()->mFoveatedOffsetY);
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Foveated Offset X", &SkyrimUpscaler::GetSingleton()->mFoveatedOffsetX, 0.01f, -1.0f, 1.0f)) {
			SkyrimUpscaler::GetSingleton()->SetupD3DBox(SkyrimUpscaler::GetSingleton()->mFoveatedOffsetX, SkyrimUpscaler::GetSingleton()->mFoveatedOffsetY);
		}
		if (ImGui::ArrowButton("mFoveatedOffsetY+", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mFoveatedOffsetY -= 0.01f;
			SkyrimUpscaler::GetSingleton()->SetupD3DBox(SkyrimUpscaler::GetSingleton()->mFoveatedOffsetX, SkyrimUpscaler::GetSingleton()->mFoveatedOffsetY);
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mFoveatedOffsetY-", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mFoveatedOffsetY += 0.01f;
			SkyrimUpscaler::GetSingleton()->SetupD3DBox(SkyrimUpscaler::GetSingleton()->mFoveatedOffsetX, SkyrimUpscaler::GetSingleton()->mFoveatedOffsetY);
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Foveated Offset Y", &SkyrimUpscaler::GetSingleton()->mFoveatedOffsetY, 0.01f, -1.0f, 1.0f)) {
			SkyrimUpscaler::GetSingleton()->SetupD3DBox(SkyrimUpscaler::GetSingleton()->mFoveatedOffsetX, SkyrimUpscaler::GetSingleton()->mFoveatedOffsetY);
		}
		ImGui::Spacing();
		ImGui::Checkbox("Enable Fixed Foveated Rendering", &SkyrimUpscaler::GetSingleton()->mVRS->mEnableFixedFoveatedRendering);
		//Inner Radius
		if (ImGui::ArrowButton("mVRS->mInnerRadius+", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mInnerRadius -= 0.01f;
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mVRS->mInnerRadius-", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mInnerRadius += 0.01f;
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Inner Radius", &SkyrimUpscaler::GetSingleton()->mVRS->mInnerRadius, 0.01f, 0.0f, 1.0f)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		};
		//Middle Radius
		if (ImGui::ArrowButton("mVRS->mMiddleRadius+", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mMiddleRadius -= 0.01f;
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mVRS->mMiddleRadius-", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mMiddleRadius += 0.01f;
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Middle Radius", &SkyrimUpscaler::GetSingleton()->mVRS->mMiddleRadius, 0.01f, 0.0f, 1.2f)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		};
		//Outter Radius
		if (ImGui::ArrowButton("mVRS->mOutterRadius+", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mOutterRadius -= 0.01f;
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mVRS->mOutterRadius-", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mOutterRadius += 0.01f;
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Outter Radius", &SkyrimUpscaler::GetSingleton()->mVRS->mOutterRadius, 0.01f, 0.0f, 1.5f)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		};
		//Cutout Radius
		if (ImGui::ArrowButton("mVRS->mCutoutRadius+", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mCutoutRadius -= 0.01f;
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mVRS->mCutoutRadius-", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mCutoutRadius += 0.01f;
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Cutout Radius", &SkyrimUpscaler::GetSingleton()->mVRS->mCutoutRadius, 0.01f, 0.0f, 1.5f)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		};
		//Widden
		if (ImGui::ArrowButton("mVRS->mWiden+", ImGuiDir_Left)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mWiden -= 0.01f;
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mVRS->mWiden-", ImGuiDir_Right)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mWiden += 0.01f;
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Widen", &SkyrimUpscaler::GetSingleton()->mVRS->mWiden, 0.01f, 0.1f, 4.0f)) {
			SkyrimUpscaler::GetSingleton()->mVRS->mNeedUpdate = true;
		};
		if (SkyrimUpscaler::GetSingleton()->mVRS->mEnableFixedFoveatedRendering) {
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::Checkbox("Enable Debug Overlay", &SkyrimUpscaler::GetSingleton()->mDebugOverlay);
			if (SkyrimUpscaler::GetSingleton()->mVRS->combinedVRSShowTex.mImage) {
				ImVec2 vec = GetTextureDimensions(SkyrimUpscaler::GetSingleton()->mVRS->combinedVRSShowTex.GetSRV());
				ImGui::Image((void*)SkyrimUpscaler::GetSingleton()->mVRS->combinedVRSShowTex.GetSRV(), vec);
			}
		}
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (sorted_item_list.empty()) {
			ImGui::TextUnformatted("No motion vectors found.");
		} else {
			std::sort(sorted_item_list.begin(), sorted_item_list.end(), [](const motion_item& a, const motion_item& b) {
				return (a.display_count > b.display_count) ||
				       (a.display_count == b.display_count && ((a.desc.Width > b.desc.Width || (a.desc.Width == b.desc.Width && a.desc.Height > b.desc.Height)) ||
																  (a.desc.Width == b.desc.Width && a.desc.Height == b.desc.Height && a.resource < b.resource)));
			});

			for (const motion_item& item : sorted_item_list) {
				char label[512] = "";
				sprintf_s(label, "%c 0x%016llx", ' ', item.resource);

				if (bool value = selected_item.resource == item.resource;
					ImGui::Checkbox(label, &value)) {
					if (value) {
						selected_item = item;
						SkyrimUpscaler::GetSingleton()->SetupMotionVector(selected_item.resource);
					}
				}
				ImGui::SameLine();
				ImGui::Text("| %4ux%-4u | %s ",
					item.desc.Width,
					item.desc.Height,
					format_to_string(item.desc.Format));
			}
		}

		ImGui::End();
	}

	// Rendering
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void SettingGUI::OnCleanup()
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void SettingGUI::toggle()
{
	ForceEnabled(!mShowGUI);
	SkyrimUpscaler::GetSingleton()->SaveINI();
}

// Codes below are from CatHub
// https://github.com/Pentalimbed/cathub
class CharEvent : public RE::InputEvent
{
public:
	uint32_t keyCode;  // 18 (ascii code)
};

RE::BSEventNotifyControl InputListener::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_eventSource)
{
	if (!a_event || !a_eventSource)
		return RE::BSEventNotifyControl::kContinue;

	auto& io = ImGui::GetIO();

	for (auto event = *a_event; event; event = event->next) {
		if (event->eventType == RE::INPUT_EVENT_TYPE::kChar) {
			io.AddInputCharacter(static_cast<CharEvent*>(event)->keyCode);
		} else if (event->eventType == RE::INPUT_EVENT_TYPE::kButton) {
			const auto button = static_cast<RE::ButtonEvent*>(event);
			if (!button || (button->IsPressed() && !button->IsDown()))
				continue;

			auto     scan_code = button->GetIDCode();
			uint32_t key = MapVirtualKeyEx(scan_code, MAPVK_VSC_TO_VK_EX, GetKeyboardLayout(0));
			switch (scan_code) {
			case DIK_LEFTARROW:
				key = VK_LEFT;
				break;
			case DIK_RIGHTARROW:
				key = VK_RIGHT;
				break;
			case DIK_UPARROW:
				key = VK_UP;
				break;
			case DIK_DOWNARROW:
				key = VK_DOWN;
				break;
			case DIK_DELETE:
				key = VK_DELETE;
				break;
			case DIK_END:
				key = VK_END;
				break;
			case DIK_HOME:
				key = VK_HOME;
				break;  // pos1
			case DIK_PRIOR:
				key = VK_PRIOR;
				break;  // page up
			case DIK_NEXT:
				key = VK_NEXT;
				break;  // page down
			case DIK_INSERT:
				key = VK_INSERT;
				break;
			case DIK_NUMPAD0:
				key = VK_NUMPAD0;
				break;
			case DIK_NUMPAD1:
				key = VK_NUMPAD1;
				break;
			case DIK_NUMPAD2:
				key = VK_NUMPAD2;
				break;
			case DIK_NUMPAD3:
				key = VK_NUMPAD3;
				break;
			case DIK_NUMPAD4:
				key = VK_NUMPAD4;
				break;
			case DIK_NUMPAD5:
				key = VK_NUMPAD5;
				break;
			case DIK_NUMPAD6:
				key = VK_NUMPAD6;
				break;
			case DIK_NUMPAD7:
				key = VK_NUMPAD7;
				break;
			case DIK_NUMPAD8:
				key = VK_NUMPAD8;
				break;
			case DIK_NUMPAD9:
				key = VK_NUMPAD9;
				break;
			case DIK_DECIMAL:
				key = VK_DECIMAL;
				break;
			case DIK_NUMPADENTER:
				key = IM_VK_KEYPAD_ENTER;
				break;
			case DIK_RMENU:
				key = VK_RMENU;
				break;  // right alt
			case DIK_RCONTROL:
				key = VK_RCONTROL;
				break;  // right control
			case DIK_LWIN:
				key = VK_LWIN;
				break;  // left win
			case DIK_RWIN:
				key = VK_RWIN;
				break;  // right win
			case DIK_APPS:
				key = VK_APPS;
				break;
			default:
				break;
			}

			switch (button->device.get()) {
			case RE::INPUT_DEVICE::kMouse:
				if (scan_code > 7)  // middle scroll
					io.AddMouseWheelEvent(0, button->Value() * (scan_code == 8 ? 1 : -1));
				else {
					if (scan_code > 5)
						scan_code = 5;
					io.AddMouseButtonEvent(scan_code, button->IsPressed());
				}
				break;
			case RE::INPUT_DEVICE::kKeyboard:
				io.AddKeyEvent(VirtualKeyToImGuiKey(key), button->IsPressed());
				break;
			case RE::INPUT_DEVICE::kGamepad:
				// not implemented yet
				// key = GetGamepadIndex((RE::BSWin32GamepadDevice::Key)key);
				break;
			default:
				continue;
			}
		}
	}

	return RE::BSEventNotifyControl::kContinue;
}

void ProcessEvent(ImGuiKey key, ImGuiKey key2)
{
	auto&       io = ImGui::GetIO();
	static bool leftButton = false;
	if (GetAsyncKeyState(VK_LBUTTON) < 0 && leftButton == false) {
		leftButton = true;
		io.AddMouseButtonEvent(0, leftButton);
	}
	if (GetAsyncKeyState(VK_LBUTTON) == 0 && leftButton == true) {
		leftButton = false;
		io.AddMouseButtonEvent(0, leftButton);
	}

	WPARAM keys[4] = {
		ImGuiKeyToVirtualKey(key),
		ImGuiKeyToVirtualKey(key2),
		VK_DELETE,
		VK_BACK
	};
	static bool pressed[4]{ false };
	for (int i = 0; i < 4; i++) {
		if (GetAsyncKeyState(keys[i]) < 0 && pressed[i] == false) {
			pressed[i] = true;
			io.AddKeyEvent(VirtualKeyToImGuiKey(keys[i]), true);
		}
		if (GetAsyncKeyState(keys[i]) == 0 && pressed[i] == true) {
			pressed[i] = false;
			io.AddKeyEvent(VirtualKeyToImGuiKey(keys[i]), false);
		}
	}
}
