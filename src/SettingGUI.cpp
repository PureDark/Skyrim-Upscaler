#include "SettingGUI.h"
#include <SkyrimUpscaler.h>

static auto upscaler = SkyrimUpscaler::GetSingleton();

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
	if (upscaler->mDelayInit > 0 && --upscaler->mDelayInit == 0) {
		upscaler->mDelayInit = -1;
		upscaler->InitUpscaler();
		upscaler->SaveINI();
	}

	if (REL::Module::IsVR()) {
		ProcessEvent((ImGuiKey)mToggleHotkey, (ImGuiKey)upscaler->mToggleUpscaler);
	}

	if (ImGui::IsKeyReleased((ImGuiKey)mToggleHotkey))
		toggle();
	if (ImGui::IsKeyReleased((ImGuiKey)upscaler->mToggleUpscaler))
		upscaler->SetEnabled(!upscaler->mEnableUpscaler);

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
				controlMap->ToggleControls(RE::UserEvents::USER_EVENT_FLAG::kAll, !mShowGUI);
			lastShowGUI = mShowGUI;
		}
	}

	if (mShowGUI) {
		// Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Begin("Skyrim Upscaler Settings", &mShowGUI, ImGuiWindowFlags_NoCollapse);
		//ImGui::SetWindowSize(ImVec2(576, 340), 0.9f);
		if (ImGui::Checkbox("Enable Upscaling", &upscaler->mEnableUpscaler)) {
			upscaler->SetEnabled(upscaler->mEnableUpscaler);
			upscaler->SaveINI();
		}
		//ImGui::Checkbox("Disable Evaluation", &upscaler->mDisableEvaluation);
		//ImGui::Checkbox("Cancel Jitter", &upscaler->mCancelJitter);
		if (ImGui::Checkbox("Use TAA For Periphery", &upscaler->mUseTAAForPeriphery)) {
			UnkOuterStruct::GetSingleton()->SetTAA(upscaler->mUseTAAForPeriphery);
			upscaler->mNeedUpdate = true;
			upscaler->SaveINI();
		}
		if (ImGui::Checkbox("Upscale Depth For ReShade", &upscaler->mUpscaleDepthForReShade)) {
			upscaler->SaveINI();
		}

		if (upscaler->mUpscaleType == FSR3) {
			if (ImGui::Checkbox("Show FSR3 Debug View", &upscaler->mDebug)) {
				SetDebug(upscaler->mDebug);
			}
		}

		//if (ImGui::Checkbox("ENB Eye Adaption Fix", &upscaler->mENBEyeAdaptionFix)) {
		//	upscaler->SaveINI();
		//}
		//ImGui::Checkbox("Debug", &upscaler->mDebug);
		//ImGui::Checkbox("Debug2", &upscaler->mDebug2);
		//ImGui::Checkbox("Debug3", &upscaler->mDebug3);
		//ImGui::Checkbox("Debug4", &upscaler->mDebug4);
		//ImGui::Checkbox("Debug5", &upscaler->mDebug5);
		//ImGui::Checkbox("Debug6", &upscaler->mDebug6);
		//ImGui::Checkbox("Blur Edges", &upscaler->mBlurEdges);
		//ImGui::BeginDisabled(!upscaler->mBlurEdges);
		//ImGui::DragFloat("Blur Intensity", &upscaler->mBlurIntensity, 0.1f, 0.0f, 10.0f);
		//ImGui::EndDisabled();

		//ImGui::DragFloat("Motion Sensitivity", &upscaler->mMotionSensitivity, 0.1f, 0.0f, 10.0f);
		//ImGui::DragFloat("Blend Scale", &upscaler->mBlendScale, 0.1f, 0.0f, 10.0f);
		//ImGui::Checkbox("Jitter", &upscaler->mEnableJitter);
		//if (ImGui::ArrowButton("mJitterPhase-", ImGuiDir_Left)) {
		//	upscaler->mJitterPhase--;
		//}
		//ImGui::SameLine(0, 4.0f);
		//if (ImGui::ArrowButton("mJitterPhase+", ImGuiDir_Right)) {
		//	upscaler->mJitterPhase++;
		//}
		//ImGui::SameLine(0, 4.0f);
		//ImGui::DragInt("Jitter Phase Count", &upscaler->mJitterPhase, 1, 0, 128);
		//ImGui::Checkbox("Enable Transparency Mask", &upscaler->mEnableTransparencyMask);
		if (ImGui::Checkbox("Sharpness", &upscaler->mSharpening)) {
			upscaler->InitUpscaler();
		}
		if (ImGui::ArrowButton("mSharpness-", ImGuiDir_Left)) {
			upscaler->mSharpness -= 0.01f;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mSharpness+", ImGuiDir_Right)) {
			upscaler->mSharpness += 0.01f;
		}
		ImGui::SameLine(0, 4.0f);
		ImGui::DragFloat("Sharpness Amount", &upscaler->mSharpness, 0.01f, 0.0f, 5.0f);

		if (ImGui::Checkbox("Use Optimal Mip Lod Bias", &upscaler->mUseOptimalMipLodBias)) {
			if (upscaler->mUseOptimalMipLodBias) {
				if (upscaler->mUpscaleType < TAA)
					upscaler->mMipLodBias = GetOptimalMipmapBias(0);
			}
		}
		ImGui::BeginDisabled(upscaler->mUseOptimalMipLodBias);
		ImGui::DragFloat("Mip Lod Bias", &upscaler->mMipLodBias, 0.1f, -3.0f, 3.0f);
		if (ImGui::Button(" -2 ")) {
			upscaler->mMipLodBias = -2;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::Button(" -1.5 ")) {
			upscaler->mMipLodBias = -1.5f;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::Button(" -1 ")) {
			upscaler->mMipLodBias = -1;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::Button(" -0.5 ")) {
			upscaler->mMipLodBias = -0.5f;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::Button(" 0 ")) {
			upscaler->mMipLodBias = 0;
		}
		ImGui::EndDisabled();

		const bool DLSSAvailable = IsUpscaleMethodAvailable(DLSS);
		const bool FSR2Available = false;
		const bool XeSSAvailable = IsUpscaleMethodAvailable(XESS);
		const bool FSR3Available = IsUpscaleMethodAvailable(FSR3);

		static std::map<int, int>       upscalerMap;
		static std::vector<std::string> imgui_combo_names{};
		static std::string              imgui_combo_names_joined{};
		if (upscalerMap.size() == 0) {
			if (DLSSAvailable) {
				upscalerMap.insert({ DLSS, imgui_combo_names.size() });
				auto str = "DLSS";
				imgui_combo_names.push_back(str);
				imgui_combo_names_joined.append(str + "\0"s);
			}
			if (FSR2Available) {
				upscalerMap.insert({ FSR2, imgui_combo_names.size() });
				auto str = "FSR2";
				imgui_combo_names.push_back(str);
				imgui_combo_names_joined.append(str + "\0"s);
			}
			if (XeSSAvailable) {
				upscalerMap.insert({ XESS, imgui_combo_names.size() });
				auto str = "XeSS";
				imgui_combo_names.push_back(str);
				imgui_combo_names_joined.append(str + "\0"s);
			}
			if (FSR3Available) {
				upscalerMap.insert({ FSR3, imgui_combo_names.size() });
				auto str = "FSR3.1 Upscaling";
				imgui_combo_names.push_back(str);
				imgui_combo_names_joined.append(str + "\0"s);
			}
		}

		int upscaleType = upscalerMap[upscaler->mUpscaleType];
		if (ImGui::Combo("Upscaling Type", (int*)&upscaleType, imgui_combo_names_joined.c_str(), imgui_combo_names.size())) {
			if (upscaleType < 0 || upscaleType >= imgui_combo_names.size()) {
				upscaleType = 0;
			}
			if (strcmp(imgui_combo_names[upscaleType].c_str(), "DLSS") == 0)
				upscaler->mUpscaleType = DLSS;
			else if (strcmp(imgui_combo_names[upscaleType].c_str(), "FSR2") == 0)
				upscaler->mUpscaleType = FSR2;
			else if (strcmp(imgui_combo_names[upscaleType].c_str(), "XeSS") == 0)
				upscaler->mUpscaleType = XESS;
			else if (strcmp(imgui_combo_names[upscaleType].c_str(), "FSR3.1 Upscaling") == 0)
				upscaler->mUpscaleType = FSR3;
			upscaler->mDebug = false;
			upscaler->SaveINI();
			upscaler->InitUpscaler();
		}

		static std::map<int, int>       qualityMap;
		static std::vector<std::string> quality_combo_names{};
		static std::string              quality_combo_names_joined{};
		if (qualityMap.size() == 0) {
			qualityMap.clear();
			quality_combo_names.clear();
			quality_combo_names_joined.clear();
			qualityMap.insert({ UltraPerformance, quality_combo_names.size() });
			auto str = "Ultra Performance";
			quality_combo_names.push_back(str);
			quality_combo_names_joined.append(str + "\0"s);
			qualityMap.insert({ Performance, quality_combo_names.size() });
			str = "Performance";
			quality_combo_names.push_back(str);
			quality_combo_names_joined.append(str + "\0"s);
			qualityMap.insert({ Balanced, quality_combo_names.size() });
			str = "Balanced";
			quality_combo_names.push_back(str);
			quality_combo_names_joined.append(str + "\0"s);
			qualityMap.insert({ Quality, quality_combo_names.size() });
			str = "Quality";
			quality_combo_names.push_back(str);
			quality_combo_names_joined.append(str + "\0"s);
			qualityMap.insert({ UltraQuality, quality_combo_names.size() });
			str = "Ultra Quality";
			quality_combo_names.push_back(str);
			quality_combo_names_joined.append(str + "\0"s);
			qualityMap.insert({ Native, quality_combo_names.size() });
			str = "Native";
			quality_combo_names.push_back(str);
			quality_combo_names_joined.append(str + "\0"s);
		}

		int qualityLevel = qualityMap[upscaler->mQualityLevel];
		ImGui::BeginDisabled(upscaler->mUpscaleType == TAA);
		if (ImGui::Combo("Quality Level", (int*)&qualityLevel, quality_combo_names_joined.c_str(), quality_combo_names.size())) {
			if (qualityLevel < 0 || qualityLevel >= quality_combo_names.size()) {
				qualityLevel = 0;
			}
			if (strcmp(quality_combo_names[qualityLevel].c_str(), "Performance") == 0)
				upscaler->mQualityLevel = Performance;
			else if (strcmp(quality_combo_names[qualityLevel].c_str(), "Balanced") == 0)
				upscaler->mQualityLevel = Balanced;
			else if (strcmp(quality_combo_names[qualityLevel].c_str(), "Quality") == 0)
				upscaler->mQualityLevel = Quality;
			else if (strcmp(quality_combo_names[qualityLevel].c_str(), "Ultra Quality") == 0)
				upscaler->mQualityLevel = UltraQuality;
			else if (strcmp(quality_combo_names[qualityLevel].c_str(), "Ultra Performance") == 0)
				upscaler->mQualityLevel = UltraPerformance;
			else if (strcmp(quality_combo_names[qualityLevel].c_str(), "Native") == 0)
				upscaler->mQualityLevel = Native;
			upscaler->InitUpscaler();
			upscaler->SaveINI();
		}
		ImGui::EndDisabled();
		if (upscaler->mUpscaleType == FSR3) {
			ImGui::DragFloat("FOV", &upscaler->mFOV, 0.1f, 0.0f, 150.0f);
			//ImGui::DragFloat("Near Plane", &upscaler->mNearPlane, 0.1f, 0.0f, 300.0f);
			//ImGui::DragFloat("Far Plane", &upscaler->mFarPlane, 1.0f, 0.0f, 10000.0f);
		}
		//const auto w = (float)GetRenderWidth(0);
		//const auto h = (float)GetRenderHeight(0);

		//if (ImGui::DragFloat("MotionScale X", &upscaler->mMotionScale[0], 0.01f, -w, w) ||
		//	ImGui::DragFloat("MotionScale Y", &upscaler->mMotionScale[1], 0.01f, -h, h)) {
		//	upscaler->SetMotionScale(upscaler->mMotionScale[0], upscaler->mMotionScale[1]);
		//}

		ImGui::Spacing();
		if (ImGui::ArrowButton("mFoveatedScaleX-", ImGuiDir_Left)) {
			upscaler->mFoveatedScaleX -= 0.01f;
			upscaler->InitUpscaler(true);
			upscaler->mDelayInit = 15;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mFoveatedScaleX+", ImGuiDir_Right)) {
			upscaler->mFoveatedScaleX += 0.01f;
			upscaler->InitUpscaler(true);
			upscaler->mDelayInit = 15;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Foveated Scale X", &upscaler->mFoveatedScaleX, 0.01f, 0.3f, 1.0f)) {
			upscaler->InitUpscaler(true);
			upscaler->mDelayInit = 15;
		}
		if (ImGui::ArrowButton("mFoveatedScaleY-", ImGuiDir_Left)) {
			upscaler->mFoveatedScaleY -= 0.01f;
			upscaler->InitUpscaler(true);
			upscaler->mDelayInit = 15;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mFoveatedScaleY+", ImGuiDir_Right)) {
			upscaler->mFoveatedScaleY += 0.01f;
			upscaler->InitUpscaler(true);
			upscaler->mDelayInit = 15;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Foveated Scale Y", &upscaler->mFoveatedScaleY, 0.01f, 0.3f, 1.0f)) {
			upscaler->InitUpscaler(true);
			upscaler->mDelayInit = 15;
		}
		if (ImGui::ArrowButton("mFoveatedOffsetX-", ImGuiDir_Left)) {
			upscaler->mFoveatedOffsetX -= 0.01f;
			upscaler->SetupD3DBox(upscaler->mFoveatedOffsetX, upscaler->mFoveatedOffsetY);
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mFoveatedOffsetX+", ImGuiDir_Right)) {
			upscaler->mFoveatedOffsetX += 0.01f;
			upscaler->SetupD3DBox(upscaler->mFoveatedOffsetX, upscaler->mFoveatedOffsetY);
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Foveated Offset X", &upscaler->mFoveatedOffsetX, 0.01f, -1.0f, 1.0f)) {
			upscaler->SetupD3DBox(upscaler->mFoveatedOffsetX, upscaler->mFoveatedOffsetY);
		}
		if (ImGui::ArrowButton("mFoveatedOffsetY+", ImGuiDir_Left)) {
			upscaler->mFoveatedOffsetY -= 0.01f;
			upscaler->SetupD3DBox(upscaler->mFoveatedOffsetX, upscaler->mFoveatedOffsetY);
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mFoveatedOffsetY-", ImGuiDir_Right)) {
			upscaler->mFoveatedOffsetY += 0.01f;
			upscaler->SetupD3DBox(upscaler->mFoveatedOffsetX, upscaler->mFoveatedOffsetY);
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Foveated Offset Y", &upscaler->mFoveatedOffsetY, 0.01f, -1.0f, 1.0f)) {
			upscaler->SetupD3DBox(upscaler->mFoveatedOffsetX, upscaler->mFoveatedOffsetY);
		}
		ImGui::Spacing();
		ImGui::Checkbox("Enable Fixed Foveated Rendering", &upscaler->mVRS->mEnableFixedFoveatedRendering);
		//Inner Radius
		if (ImGui::ArrowButton("mVRS->mInnerRadius+", ImGuiDir_Left)) {
			upscaler->mVRS->mInnerRadius -= 0.01f;
			upscaler->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mVRS->mInnerRadius-", ImGuiDir_Right)) {
			upscaler->mVRS->mInnerRadius += 0.01f;
			upscaler->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Inner Radius", &upscaler->mVRS->mInnerRadius, 0.01f, 0.0f, 1.0f)) {
			upscaler->mVRS->mNeedUpdate = true;
		};
		//Middle Radius
		if (ImGui::ArrowButton("mVRS->mMiddleRadius+", ImGuiDir_Left)) {
			upscaler->mVRS->mMiddleRadius -= 0.01f;
			upscaler->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mVRS->mMiddleRadius-", ImGuiDir_Right)) {
			upscaler->mVRS->mMiddleRadius += 0.01f;
			upscaler->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Middle Radius", &upscaler->mVRS->mMiddleRadius, 0.01f, 0.0f, 1.2f)) {
			upscaler->mVRS->mNeedUpdate = true;
		};
		//Outter Radius
		if (ImGui::ArrowButton("mVRS->mOutterRadius+", ImGuiDir_Left)) {
			upscaler->mVRS->mOuterRadius -= 0.01f;
			upscaler->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mVRS->mOutterRadius-", ImGuiDir_Right)) {
			upscaler->mVRS->mOuterRadius += 0.01f;
			upscaler->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Outter Radius", &upscaler->mVRS->mOuterRadius, 0.01f, 0.0f, 1.5f)) {
			upscaler->mVRS->mNeedUpdate = true;
		};
		//Cutout Radius
		if (ImGui::ArrowButton("mVRS->mCutoutRadius+", ImGuiDir_Left)) {
			upscaler->mVRS->mCutoutRadius -= 0.01f;
			upscaler->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mVRS->mCutoutRadius-", ImGuiDir_Right)) {
			upscaler->mVRS->mCutoutRadius += 0.01f;
			upscaler->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Cutout Radius", &upscaler->mVRS->mCutoutRadius, 0.01f, 0.0f, 1.5f)) {
			upscaler->mVRS->mNeedUpdate = true;
		};
		//Widden
		if (ImGui::ArrowButton("mVRS->mWiden+", ImGuiDir_Left)) {
			upscaler->mVRS->mWiden -= 0.01f;
			upscaler->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::ArrowButton("mVRS->mWiden-", ImGuiDir_Right)) {
			upscaler->mVRS->mWiden += 0.01f;
			upscaler->mVRS->mNeedUpdate = true;
		}
		ImGui::SameLine(0, 4.0f);
		if (ImGui::DragFloat("Widen", &upscaler->mVRS->mWiden, 0.01f, 0.1f, 4.0f)) {
			upscaler->mVRS->mNeedUpdate = true;
		};
		if (upscaler->mVRS->mEnableFixedFoveatedRendering) {
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::Checkbox("Enable Debug Overlay", &upscaler->mDebugOverlay);
			if (upscaler->mVRS->combinedVRSShowTex.mImage) {
				ImVec2 vec = GetTextureDimensions(upscaler->mVRS->combinedVRSShowTex.GetSRV());
				ImGui::Image((void*)upscaler->mVRS->combinedVRSShowTex.GetSRV(), vec);
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
						upscaler->SetupMotionVector(selected_item.resource);
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
	upscaler->SaveINI();
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
