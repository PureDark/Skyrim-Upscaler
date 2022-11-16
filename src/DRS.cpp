#include "DRS.h"

#include <SkyrimUpscaler.h>

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

void DRS::SetDRS(BSGraphics::State* a_state)
{
	ResetScale();
	auto& runtimeData = a_state->GetRuntimeData();
	runtimeData.fDynamicResolutionCurrentHeightScale = currentScaleFactor;
	runtimeData.fDynamicResolutionCurrentWidthScale = currentScaleFactor;
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
