//#pragma once
//
//#include <imgui.h>
//#include <reshade.hpp>
//#include <cmath>
//#include <cstring>
//#include <unordered_map>
//#include <SkyrimUpscaler.h>
//#include <d3d11_4.h>
//#include <DRS.h>
//#include <thread>
//#include <algorithm>
//#include <PCH.h>
//#include <unordered_set>
//
//using namespace reshade::api;
//
//static reshade::api::swapchain* m_swapchain = nullptr;
//
//struct depth_stencil_hash
//{
//	inline size_t operator()(resource value) const
//	{
//		// Simply use the handle (which is usually a pointer) as hash value (with some bits shaved off due to pointer alignment)
//		return static_cast<size_t>(value.handle >> 4);
//	}
//};
//
//struct __declspec(uuid("7c6363c7-f94e-437a-9160-141782c44a98")) generic_depth_data
//{
//	// The depth-stencil resource that is currently selected as being the main depth target
//	resource selected_depth_stencil = { 0 };
//
//	// Resource used to override automatic depth-stencil selection
//	resource override_depth_stencil = { 0 };
//
//	// The current depth shader resource view bound to shaders
//	// This can be created from either the selected depth-stencil resource (if it supports shader access) or from a backup resource
//	resource_view selected_shader_resource = { 0 };
//
//	// True when the shader resource view was created from the backup resource, false when it was created from the original depth-stencil
//	bool using_backup_texture = false;
//
//	std::unordered_map<resource, unsigned int, depth_stencil_hash> display_count_per_depth_stencil;
//};
//
//struct motion_item
//{
//	unsigned int  display_count;
//	resource      resource;
//	resource_desc desc;
//};
//static std::vector<motion_item> sorted_item_list;
//static motion_item              selected_item;
//static std::unordered_set<ID3D11SamplerState*>                      passThroughSamplers;
//static std::unordered_map<ID3D11SamplerState*, ID3D11SamplerState*> mappedSamplers;
//static float                                                        mipLodBias = 0;
