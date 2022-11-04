#pragma once

#include <Detours.h>
#include "RE/BSGraphics.h"
#include <PCH.h>
#include <ENB/ENBSeriesAPI.h>

extern ID3D11DeviceContext* g_DeviceContext;
extern ID3D11Device*        g_Device;
extern IDXGISwapChain*      g_SwapChain;

class MenuOpenCloseEventHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource);
	static bool                      Register();
};

class DRS
{
public:
	static DRS* GetSingleton()
	{
		static DRS handler;
		return &handler;
	}

	static void InstallHooks()
	{
		Hooks::Install();
	}

	// Settings

	RE::Setting* bEnableAutoDynamicResolution;
	void         GetGameSettings();

	bool  bEnableWithENB = false;

	// Variables

	float targetScaleFactor = 1.0f;
	float currentScaleFactor = 1.0f;
	bool  reset = false;

	void Update();
	void ControlResolution();
	void ResetScale();

	void SetDRS(BSGraphics::State* a_state);

	void MessageHandler(SKSE::MessagingInterface::Message* a_msg);

protected:
	struct Hooks
	{
		struct Main_SetDRS
		{
			static void thunk(BSGraphics::State* a_state)
			{
				func(a_state);
				GetSingleton()->SetDRS(a_state);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		static void Install()
		{
			stl::write_thunk_call<Main_SetDRS>(REL::RelocationID(35556, 36555).address() + REL::Relocate(0x2D, 0x2D, 0x25));
		}
	};

private:
	DRS(){};

	DRS(const DRS&) = delete;
	DRS(DRS&&) = delete;

	~DRS() = default;

	DRS& operator=(const DRS&) = delete;
	DRS& operator=(DRS&&) = delete;
};
