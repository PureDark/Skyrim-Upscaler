#pragma once

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>



class SettingGUI
{
public:
	//Reserve the second value for VR
	IDXGISwapChain*  mSwapChain;
	ID3D11Device*    mDevice;

	~SettingGUI() {}

	static SettingGUI* GetSingleton()
	{
		static SettingGUI handler;
		return &handler;
	}

	void InitIMGUI(IDXGISwapChain* swapchain, ID3D11Device* device, ID3D11DeviceContext* context);
	void OnRender();
	void OnCleanup();
};
