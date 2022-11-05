#include "DRS.h"

#include <ENB/AntTweakBar.h>
#include <ENB/ENBSeriesAPI.h>
#include <SkyrimUpscaler.h>
extern ENB_API::ENBSDKALT1001* g_ENB;

void DRS::GetGameSettings()
{
	bEnableAutoDynamicResolution = RE::GetINISetting("bEnableAutoDynamicResolution:Display");
	if (!REL::Module::IsVR()) {
		if (bEnableAutoDynamicResolution) {
			if (!bEnableAutoDynamicResolution->GetBool())
				logger::info("Forcing DynamicResolution on.");
			bEnableAutoDynamicResolution->data.b = true;
		} else
			logger::warn("Unable to enable Dynamic Resolution, please enable manually.");
	}
}

void DRS::Update()
{
	if (reset) {
		ResetScale();
		return;
	}
	else {
		ControlResolution();
	}
}

void DRS::ControlResolution()
{
	currentScaleFactor = SkyrimUpscaler::GetSingleton()->IsEnabled()?targetScaleFactor:1.0f;
}

void DRS::ResetScale()
{
	currentScaleFactor = 1.0f;
}

bool GetENBParameterBool(const char* a_filename, const char* a_category, const char* a_keyname)
{
	BOOL                  bvalue;
	ENB_SDK::ENBParameter param;
	if (g_ENB->GetParameter(a_filename, a_category, a_keyname, &param)) {
		if (param.Type == ENB_SDK::ENBParameterType::ENBParam_BOOL) {
			memcpy(&bvalue, param.Data, ENBParameterTypeToSize(ENB_SDK::ENBParameterType::ENBParam_BOOL));
			return bvalue;
		}
	}
	logger::debug("Could not find ENB parameter {}:{}:{}", a_filename, a_category, a_keyname);
	return false;
}

void DRS::SetDRS(BSGraphics::State* a_state)
{
	if ((!(g_ENB && GetENBParameterBool("enbseries.ini", "GLOBAL", "UseEffect")) || bEnableWithENB)) {
		a_state->fDynamicResolutionCurrentHeightScale = currentScaleFactor;
		a_state->fDynamicResolutionCurrentWidthScale = currentScaleFactor;
	} else {
		a_state->fDynamicResolutionPreviousHeightScale = 1.0f;
		a_state->fDynamicResolutionPreviousWidthScale = 1.0f;
		a_state->fDynamicResolutionCurrentHeightScale = 1.0f;
		a_state->fDynamicResolutionCurrentWidthScale = 1.0f;
	}
}

void DRS::MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		GetGameSettings();
		break;
	}
}

// Fader Menu
// Mist Menu
// Loading Menu
// LoadWaitSpinner

RE::BSEventNotifyControl MenuOpenCloseEventHandler::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
{
	if (a_event->menuName == RE::LoadingMenu::MENU_NAME ||
		a_event->menuName == RE::RaceSexMenu::MENU_NAME) {
		if (a_event->opening) {
			DRS::GetSingleton()->reset = true;
			DRS::GetSingleton()->ResetScale();
		} else {
			DRS::GetSingleton()->reset = false;
			DRS::GetSingleton()->ControlResolution();
		}
	}
	else if (a_event->menuName == RE::FaderMenu::MENU_NAME) {
		if (!a_event->opening) {
			DRS::GetSingleton()->reset = false;
			DRS::GetSingleton()->ControlResolution();
		}
	}

	return RE::BSEventNotifyControl::kContinue;
}

bool MenuOpenCloseEventHandler::Register()
{
	static MenuOpenCloseEventHandler singleton;
	auto                             ui = RE::UI::GetSingleton();

	if (!ui) {
		logger::error("UI event source not found");
		return false;
	}

	ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(&singleton);

	logger::info("Registered {}", typeid(singleton).name());

	return true;
}
