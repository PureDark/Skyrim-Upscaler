#pragma once

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <dinput.h>
#include <unordered_set>

struct motion_item
{
	unsigned int  display_count;
	ID3D11Texture2D* resource;
	D3D11_TEXTURE2D_DESC desc;
};

class SettingGUI
{
public:
	//Reserve the second value for VR
	IDXGISwapChain*          mSwapChain;
	ID3D11Device*            mDevice;
	bool                     mShowGUI{true};
	std::vector<motion_item> sorted_item_list;
	motion_item              selected_item;
	int                      mToggleHotkey{ ImGuiKey_End };

	~SettingGUI() {}

	static SettingGUI* GetSingleton()
	{
		static SettingGUI handler;
		return &handler;
	}

	void toggle()
	{
		ForceEnabled(!mShowGUI);
	}

	void ForceEnabled(bool enabled)
	{
		mShowGUI = enabled;
	}

	void InitIMGUI(IDXGISwapChain* swapchain, ID3D11Device* device, ID3D11DeviceContext* context);
	void OnRender();
	void OnCleanup();
};

struct WndProcHook
{
	static LRESULT        thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static inline WNDPROC func;
};

class InputListener : public RE::BSTEventSink<RE::InputEvent*>
{
public:
	static InputListener* GetSingleton()
	{
		static InputListener listener;
		return std::addressof(listener);
	}

	virtual RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;
};

template <typename _MapType>
auto get_map_key_value(const _MapType& input_map, const decltype(input_map.begin()->second)& mapped_value) -> decltype(input_map.begin()->first)
{
	auto iter = std::find_if(input_map.begin(), input_map.end(), [mapped_value](const auto& item) {
		return (item.second == mapped_value);
	});

	if (iter == input_map.end()) {
		return decltype(input_map.begin()->first)();
	}
	return iter->first;
}

#define IM_VK_KEYPAD_ENTER (VK_RETURN + 256)
static std::map<WPARAM, ImGuiKey> keyMap = {
	{ VK_TAB, ImGuiKey_Tab },
	{ VK_LEFT, ImGuiKey_LeftArrow },
	{ VK_RIGHT, ImGuiKey_RightArrow },
	{ VK_UP, ImGuiKey_UpArrow },
	{ VK_DOWN, ImGuiKey_DownArrow },
	{ VK_PRIOR, ImGuiKey_PageUp },
	{ VK_NEXT, ImGuiKey_PageDown },
	{ VK_HOME, ImGuiKey_Home },
	{ VK_END, ImGuiKey_End },
	{ VK_INSERT, ImGuiKey_Insert },
	{ VK_DELETE, ImGuiKey_Delete },
	{ VK_BACK, ImGuiKey_Backspace },
	{ VK_SPACE, ImGuiKey_Space },
	{ VK_RETURN, ImGuiKey_Enter },
	{ VK_ESCAPE, ImGuiKey_Escape },
	{ VK_OEM_7, ImGuiKey_Apostrophe },
	{ VK_OEM_COMMA, ImGuiKey_Comma },
	{ VK_OEM_MINUS, ImGuiKey_Minus },
	{ VK_OEM_PERIOD, ImGuiKey_Period },
	{ VK_OEM_2, ImGuiKey_Slash },
	{ VK_OEM_1, ImGuiKey_Semicolon },
	{ VK_OEM_PLUS, ImGuiKey_Equal },
	{ VK_OEM_4, ImGuiKey_LeftBracket },
	{ VK_OEM_5, ImGuiKey_Backslash },
	{ VK_OEM_6, ImGuiKey_RightBracket },
	{ VK_OEM_3, ImGuiKey_GraveAccent },
	{ VK_CAPITAL, ImGuiKey_CapsLock },
	{ VK_SCROLL, ImGuiKey_ScrollLock },
	{ VK_NUMLOCK, ImGuiKey_NumLock },
	{ VK_SNAPSHOT, ImGuiKey_PrintScreen },
	{ VK_PAUSE, ImGuiKey_Pause },
	{ VK_NUMPAD0, ImGuiKey_Keypad0 },
	{ VK_NUMPAD1, ImGuiKey_Keypad1 },
	{ VK_NUMPAD2, ImGuiKey_Keypad2 },
	{ VK_NUMPAD3, ImGuiKey_Keypad3 },
	{ VK_NUMPAD4, ImGuiKey_Keypad4 },
	{ VK_NUMPAD5, ImGuiKey_Keypad5 },
	{ VK_NUMPAD6, ImGuiKey_Keypad6 },
	{ VK_NUMPAD7, ImGuiKey_Keypad7 },
	{ VK_NUMPAD8, ImGuiKey_Keypad8 },
	{ VK_NUMPAD9, ImGuiKey_Keypad9 },
	{ VK_DECIMAL, ImGuiKey_KeypadDecimal },
	{ VK_DIVIDE, ImGuiKey_KeypadDivide },
	{ VK_MULTIPLY, ImGuiKey_KeypadMultiply },
	{ VK_SUBTRACT, ImGuiKey_KeypadSubtract },
	{ VK_ADD, ImGuiKey_KeypadAdd },
	{ IM_VK_KEYPAD_ENTER, ImGuiKey_KeypadEnter },
	{ VK_LSHIFT, ImGuiKey_LeftShift },
	{ VK_LCONTROL, ImGuiKey_LeftCtrl },
	{ VK_LMENU, ImGuiKey_LeftAlt },
	{ VK_LWIN, ImGuiKey_LeftSuper },
	{ VK_RSHIFT, ImGuiKey_RightShift },
	{ VK_RCONTROL, ImGuiKey_RightCtrl },
	{ VK_RMENU, ImGuiKey_RightAlt },
	{ VK_RWIN, ImGuiKey_RightSuper },
	{ VK_APPS, ImGuiKey_Menu },
	{ '0', ImGuiKey_0 },
	{ '1', ImGuiKey_1 },
	{ '2', ImGuiKey_2 },
	{ '3', ImGuiKey_3 },
	{ '4', ImGuiKey_4 },
	{ '5', ImGuiKey_5 },
	{ '6', ImGuiKey_6 },
	{ '7', ImGuiKey_7 },
	{ '8', ImGuiKey_8 },
	{ '9', ImGuiKey_9 },
	{ 'A', ImGuiKey_A },
	{ 'B', ImGuiKey_B },
	{ 'C', ImGuiKey_C },
	{ 'D', ImGuiKey_D },
	{ 'E', ImGuiKey_E },
	{ 'F', ImGuiKey_F },
	{ 'G', ImGuiKey_G },
	{ 'H', ImGuiKey_H },
	{ 'I', ImGuiKey_I },
	{ 'J', ImGuiKey_J },
	{ 'K', ImGuiKey_K },
	{ 'L', ImGuiKey_L },
	{ 'M', ImGuiKey_M },
	{ 'N', ImGuiKey_N },
	{ 'O', ImGuiKey_O },
	{ 'P', ImGuiKey_P },
	{ 'Q', ImGuiKey_Q },
	{ 'R', ImGuiKey_R },
	{ 'S', ImGuiKey_S },
	{ 'T', ImGuiKey_T },
	{ 'U', ImGuiKey_U },
	{ 'V', ImGuiKey_V },
	{ 'W', ImGuiKey_W },
	{ 'X', ImGuiKey_X },
	{ 'Y', ImGuiKey_Y },
	{ 'Z', ImGuiKey_Z },
	{ VK_F1, ImGuiKey_F1 },
	{ VK_F2, ImGuiKey_F2 },
	{ VK_F3, ImGuiKey_F3 },
	{ VK_F4, ImGuiKey_F4 },
	{ VK_F5, ImGuiKey_F5 },
	{ VK_F6, ImGuiKey_F6 },
	{ VK_F7, ImGuiKey_F7 },
	{ VK_F8, ImGuiKey_F8 },
	{ VK_F9, ImGuiKey_F9 },
	{ VK_F10, ImGuiKey_F10 },
	{ VK_F11, ImGuiKey_F11 },
	{ VK_F12, ImGuiKey_F12 }
};
static ImGuiKey VirtualKeyToImGuiKey(WPARAM wParam)
{
	if (keyMap.contains(wParam))
		return keyMap[wParam];
	else
		return ImGuiKey_None;
}
static WPARAM ImGuiKeyToVirtualKey(ImGuiKey key)
{
	return get_map_key_value(keyMap, key);
}
