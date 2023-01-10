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

void DRS::SetDRSVR(float renderScale)
{
	if (renderScale == 0)
		renderScale = currentScaleFactor;
	auto currentWidthRatio = reinterpret_cast<float*>(REL::Offset(0x3186d14).address());
	auto currentHeightRatio = reinterpret_cast<float*>(REL::Offset(0x3186d18).address());
	auto previousWidthRatio = reinterpret_cast<float*>(REL::Offset(0x3186d1c).address());
	auto previousHeightRatio = reinterpret_cast<float*>(REL::Offset(0x3186d20).address());
	*previousWidthRatio = *currentWidthRatio;
	*previousHeightRatio = *currentHeightRatio;
	*currentWidthRatio = renderScale;
	*currentHeightRatio = renderScale;
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
	if (REL::Module::IsVR()) {
		SetDRSVR(currentScaleFactor);
	}
	auto& runtimeData = a_state->GetRuntimeData();
	runtimeData.fDynamicResolutionCurrentHeightScale = currentScaleFactor;
	runtimeData.fDynamicResolutionCurrentWidthScale = currentScaleFactor;
	lastScaleFactor = currentScaleFactor;
}

void DRS::MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		GetGameSettings();
		break;
	}
}

bool DRS::IsInFullscreenMenu()
{
	return isInMainMenu || isInLoadingMenu;
}

// Fader Menu
// Mist Menu
// Loading Menu
// LoadWaitSpinner

RE::BSEventNotifyControl MenuOpenCloseEventHandler::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
{
	if (a_event->menuName == RE::MainMenu::MENU_NAME) {
		if (a_event->opening) {
			DRS::GetSingleton()->isInMainMenu = true;
		} else {
			DRS::GetSingleton()->isInMainMenu = false;
		}
	} else if (a_event->menuName == RE::LoadingMenu::MENU_NAME) {
		if (a_event->opening) {
			DRS::GetSingleton()->isInLoadingMenu = true;
			SkyrimUpscaler::GetSingleton()->mDelayEnable = SkyrimUpscaler::GetSingleton()->mEnableUpscaler;
			if (SkyrimUpscaler::GetSingleton()->mENBEyeAdaptionFix) {
				switch (SkyrimUpscaler::GetSingleton()->mUpscaleType) {
				case DLSS:
					SkyrimUpscaler::GetSingleton()->mOriginalValue = SkyrimUpscaler::GetSingleton()->mUpscaleType;
					SkyrimUpscaler::GetSingleton()->mOriginalRenderSizeX = SkyrimUpscaler::GetSingleton()->mRenderSizeX;
					SkyrimUpscaler::GetSingleton()->mOriginalRenderSizeY = SkyrimUpscaler::GetSingleton()->mRenderSizeY;
					SkyrimUpscaler::GetSingleton()->mUpscaleType = DLAA;
					break;
				case FSR2:
				case XESS:
					//SkyrimUpscaler::GetSingleton()->mOriginalValue = SkyrimUpscaler::GetSingleton()->mQualityLevel;
					//SkyrimUpscaler::GetSingleton()->mQualityLevel = 4;
					break;
				}
				SkyrimUpscaler::GetSingleton()->mUpscaleType = DLAA;
				SkyrimUpscaler::GetSingleton()->InitUpscaler(true);
			}
			UnkOuterStruct::GetSingleton()->SetTAA(false);
			SkyrimUpscaler::GetSingleton()->mNeedUpdate = true;
		} else {
			DRS::GetSingleton()->isInLoadingMenu = false;
			if (SkyrimUpscaler::GetSingleton()->mDelayEnable)
				SkyrimUpscaler::GetSingleton()->mEnableDelayCount = 150;
		}
	}
	if (a_event->menuName == RE::MainMenu::MENU_NAME ||
		a_event->menuName == RE::LoadingMenu::MENU_NAME ||
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
