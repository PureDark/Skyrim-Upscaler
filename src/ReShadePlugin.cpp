//#include <ReShadePlugin.h>
//
//static void MyLog(char* message, int size)
//{
//	reshade::log_message(3, message);
//}
//
//static inline const char* format_to_string(format format)
//{
//	switch (format) {
//	case format::r16g16_typeless:
//		return "R16G16 Typeless";
//	case format::r16g16_uint:
//		return "R16G16 UInt";
//	case format::r16g16_sint:
//		return "R16G16 SInt";
//	case format::r16g16_float:
//		return "R16G16 Float";
//	case format::r16g16_unorm:
//		return "R16G16 UNorm";
//	case format::r16g16_snorm:
//		return "R16G16 SNorm";
//	default:
//		return "     ";
//	}
//}
//
//static void on_init_resource(device* device, const resource_desc& desc, const subresource_data*, resource_usage, resource resource)
//{
//	if (desc.type != resource_type::surface && desc.type != resource_type::texture_2d)
//		return;  // Skip resources that are not 2D textures
//	if (desc.texture.format >= format::r16g16_typeless && desc.texture.format <= format::r16g16_snorm) {
//		char str[512];
//		sprintf_s(str, "Motion Vertor Found : %d x %d", desc.texture.width, desc.texture.height);
//		reshade::log_message(3, str);
//		bool exist = false;
//		for (const motion_item& item : sorted_item_list) {
//			if (item.resource.handle == resource.handle)
//				exist = true;
//		}
//		if (!exist) {
//			motion_item item = { 1u, resource, device->get_resource_desc(resource) };
//			if (sorted_item_list.size() == 0) {
//				selected_item = item;
//				SkyrimUpscaler::GetSingleton()->SetupMotionVector((ID3D11Texture2D*)selected_item.resource.handle);
//			}
//			sorted_item_list.push_back(item);
//		}
//	}
//}
//
//static void on_init_swapchain(swapchain* swapchain)
//{
//	if (m_swapchain == nullptr) {
//		m_swapchain = swapchain;
//		SkyrimUpscaler::GetSingleton()->SetupSwapChain((IDXGISwapChain*)m_swapchain->get_native());
//
//		InitCSharpDelegate(MyLog);
//		SkyrimUpscaler::GetSingleton()->InitUpscaler();
//	}
//}
//
//static void draw_settings_overlay(effect_runtime* runtime)
//{
//	device* const device = runtime->get_device();
//
//	if (ImGui::Checkbox("Enable", &SkyrimUpscaler::GetSingleton()->mEnableUpscaler)) {
//		SkyrimUpscaler::GetSingleton()->SetEnabled(SkyrimUpscaler::GetSingleton()->mEnableUpscaler);
//	}
//	ImGui::Checkbox("Disable Evaluation", &SkyrimUpscaler::GetSingleton()->mDisableEvaluation);
//	ImGui::Checkbox("Disable Result Copying", &SkyrimUpscaler::GetSingleton()->mDisableResultCopying);
//	ImGui::Checkbox("Jitter", &SkyrimUpscaler::GetSingleton()->mEnableJitter);
//	if (ImGui::Checkbox("Sharpness", &SkyrimUpscaler::GetSingleton()->mSharpening)) {
//		SkyrimUpscaler::GetSingleton()->InitUpscaler();
//	}
//
//	ImGui::DragFloat("Sharpness Amount", &SkyrimUpscaler::GetSingleton()->mSharpness, 0.01f, 0.0f, 5.0f);
//
//	std::vector<const char*> imgui_combo_names{};
//	imgui_combo_names.push_back("DLSS");
//	imgui_combo_names.push_back("FSR2");
//	imgui_combo_names.push_back("XeSS");
//
//	if (ImGui::Combo("Upscale Type", (int*)&SkyrimUpscaler::GetSingleton()->mUpscaleType, imgui_combo_names.data(), imgui_combo_names.size())) {
//		if (SkyrimUpscaler::GetSingleton()->mUpscaleType < 0 || SkyrimUpscaler::GetSingleton()->mUpscaleType > 3) {
//			SkyrimUpscaler::GetSingleton()->mUpscaleType = 0;
//		}
//
//		std::this_thread::sleep_for(std::chrono::milliseconds(100));
//		SkyrimUpscaler::GetSingleton()->InitUpscaler();
//	}
//
//	const auto qualities = (SkyrimUpscaler::GetSingleton()->mQualityLevel == 2) ? "Performance\0Balanced\0Quality\0UltraQuality\0" : "Performance\0Balanced\0Quality\0UltraPerformance\0";
//
//	if (ImGui::Combo("Quality Level", (int*)&SkyrimUpscaler::GetSingleton()->mQualityLevel, qualities))
//	{
//		std::this_thread::sleep_for(std::chrono::milliseconds(100));
//		SkyrimUpscaler::GetSingleton()->InitUpscaler();
//	}
//
//	const auto w = (float)GetRenderWidth(0);
//	const auto h = (float)GetRenderHeight(0);
//
//	if (ImGui::DragFloat("MotionScale X", &SkyrimUpscaler::GetSingleton()->mMotionScale[0], 0.01f, -w, w) ||
//		ImGui::DragFloat("MotionScale Y", &SkyrimUpscaler::GetSingleton()->mMotionScale[1], 0.01f, -h, h)) {
//		SkyrimUpscaler::GetSingleton()->SetMotionScale(SkyrimUpscaler::GetSingleton()->mMotionScale[0], SkyrimUpscaler::GetSingleton()->mMotionScale[1]);
//	}
//
//	ImGui::Spacing();
//	ImGui::Separator();
//	ImGui::Spacing();
//
//	if (sorted_item_list.empty()) {
//		ImGui::TextUnformatted("No motion vectors found.");
//		return;
//	}
//
//	std::sort(sorted_item_list.begin(), sorted_item_list.end(), [](const motion_item& a, const motion_item& b) {
//		return (a.display_count > b.display_count) ||
//		       (a.display_count == b.display_count && ((a.desc.texture.width > b.desc.texture.width || (a.desc.texture.width == b.desc.texture.width && a.desc.texture.height > b.desc.texture.height)) ||
//														  (a.desc.texture.width == b.desc.texture.width && a.desc.texture.height == b.desc.texture.height && a.resource < b.resource)));
//	});
//
//	for (const motion_item& item : sorted_item_list) {
//		char label[512] = "";
//		sprintf_s(label, "%c 0x%016llx", ' ', item.resource.handle);
//
//		if (bool value = selected_item.resource == item.resource;
//			ImGui::Checkbox(label, &value)) {
//			if (value) {
//				selected_item = item;
//				SkyrimUpscaler::GetSingleton()->SetupMotionVector((ID3D11Texture2D*)selected_item.resource.handle);
//			}
//		}
//		ImGui::SameLine();
//		ImGui::Text("| %4ux%-4u | %s ",
//			item.desc.texture.width,
//			item.desc.texture.height,
//			format_to_string(item.desc.texture.format));
//	}
//
//	if (m_swapchain != nullptr) {
//		auto             depth = m_swapchain->get_private_data<generic_depth_data>().selected_depth_stencil;
//		ID3D11Texture2D* depth_buffer = (ID3D11Texture2D*)depth.handle;
//		SkyrimUpscaler::GetSingleton()->SetupDepth(depth_buffer);
//	}
//}
//
///* To apply mip lod bias */
//static void on_push_descriptors(command_list* cmd_list, shader_stage stages, pipeline_layout layout, uint32_t layout_param, const descriptor_set_update& update)
//{
//	if (update.type == descriptor_type::sampler) {
//		if (mipLodBias != SkyrimUpscaler::GetSingleton()->mMipLodBias) {
//			char str[512];
//			sprintf_s(str, "MIP LOD Bias changed from  %f to %f, recreating samplers", mipLodBias, SkyrimUpscaler::GetSingleton()->mMipLodBias);
//			reshade::log_message(3, str);
//			passThroughSamplers.clear();
//			mappedSamplers.clear();
//			mipLodBias = SkyrimUpscaler::GetSingleton()->mMipLodBias;
//		}
//		ID3D11SamplerState* samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
//		memcpy(samplers, update.descriptors, update.count * sizeof(ID3D11SamplerState*));
//		for (UINT i = 0; i < update.count; ++i) {
//			auto orig = samplers[i];
//			if (orig == nullptr || passThroughSamplers.find(orig) != passThroughSamplers.end()) {
//				return;
//			}
//
//			if (mappedSamplers.find(orig) == mappedSamplers.end()) {
//				D3D11_SAMPLER_DESC sd;
//				orig->GetDesc(&sd);
//				if (sd.MipLODBias != 0 || sd.MaxAnisotropy == 1) {
//					// do not mess with samplers that already have a bias or are not doing anisotropic filtering.
//					// should hopefully reduce the chance of causing rendering errors.
//					passThroughSamplers.insert(orig);
//					continue;
//				}
//				sd.MipLODBias = mipLodBias;
//				char str[512];
//				sprintf_s(str, "Creating replacement sampler with MIP LOD bias %f", mipLodBias);
//				reshade::log_message(3, str);
//
//				auto d3ddevice = (ID3D11Device*)m_swapchain->get_device()->get_native();
//				d3ddevice->CreateSamplerState(&sd, &mappedSamplers[orig]);
//				passThroughSamplers.insert(mappedSamplers[orig]);
//			}
//			samplers[i] = mappedSamplers[orig];
//		}
//		auto d3ddevice = (ID3D11Device*)m_swapchain->get_device()->get_native();
//		ID3D11DeviceContext* context;
//		d3ddevice->GetImmediateContext(&context);
//		if (stages == shader_stage::vertex)
//			context->VSSetSamplers(update.binding, update.count, samplers);
//		else if (stages == shader_stage::hull)
//			context->HSSetSamplers(update.binding, update.count, samplers);
//		else if (stages == shader_stage::domain)
//			context->DSSetSamplers(update.binding, update.count, samplers);
//		else if (stages == shader_stage::geometry)
//			context->GSSetSamplers(update.binding, update.count, samplers);
//		else if (stages == shader_stage::compute)
//			context->CSSetSamplers(update.binding, update.count, samplers);
//		else if (stages == shader_stage::pixel)
//			context->PSSetSamplers(update.binding, update.count, samplers);
//	}
//}
//
//void register_addon_upscaler()
//{
//	reshade::register_overlay(nullptr, draw_settings_overlay);
//	reshade::register_event<reshade::addon_event::init_resource>(on_init_resource);
//	reshade::register_event<reshade::addon_event::init_swapchain>(on_init_swapchain);
//	reshade::register_event<reshade::addon_event::push_descriptors>(on_push_descriptors);
//	//reshade::register_event<reshade::addon_event::present>(on_present);
//}
//void unregister_addon_upscaler()
//{
//	reshade::unregister_event<reshade::addon_event::init_resource>(on_init_resource);
//	reshade::unregister_event<reshade::addon_event::init_swapchain>(on_init_swapchain);
//	reshade::unregister_event<reshade::addon_event::push_descriptors>(on_push_descriptors);
//	//reshade::unregister_event<reshade::addon_event::present>(on_present);
//}
//
//extern "C" __declspec(dllexport) const char* NAME = "Skyrim Upscaler";
//extern "C" __declspec(dllexport) const char* DESCRIPTION = "DLSS/FSR2/XeSS.";
//
//BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
//{
//	switch (fdwReason) {
//	case DLL_PROCESS_ATTACH:
//		if (!reshade::register_addon(hModule))
//			return FALSE;
//		register_addon_upscaler();
//		break;
//	case DLL_PROCESS_DETACH:
//		unregister_addon_upscaler();
//		reshade::unregister_addon(hModule);
//		break;
//	}
//
//	return TRUE;
//}

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
