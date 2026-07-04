#include "Controls.h"

#include "ModApi.h"
extern ModApi *api;

namespace Input::Controls {

ControlAnalog movement;
ControlAnalog camera;
ControlButton jump;
ControlButton attack;
ControlButton cameraFlip;
ControlButton itemUse;
ControlButton itemPrev;
ControlButton itemNext;
ControlButton stepLeft;
ControlButton stepRight;
ControlButton pause;

void Setup() {
	movement = (ControlAnalog)api->SetupIniInt(L"InputControls", L"Movement", static_cast<int>(ControlAnalog::LeftStick));
	camera = (ControlAnalog)api->SetupIniInt(L"InputControls", L"Camera", static_cast<int>(ControlAnalog::RightStick));
	jump = (ControlButton)api->SetupIniInt(L"InputControls", L"Jump", static_cast<int>(ControlButton::A));
	attack = (ControlButton)api->SetupIniInt(L"InputControls", L"Attack", static_cast<int>(ControlButton::B));
	cameraFlip = (ControlButton)api->SetupIniInt(L"InputControls", L"CameraFlip", static_cast<int>(ControlButton::X));
	itemUse = (ControlButton)api->SetupIniInt(L"InputControls", L"ItemUse", static_cast<int>(ControlButton::Y));
	itemPrev = (ControlButton)api->SetupIniInt(L"InputControls", L"ItemPrev", static_cast<int>(ControlButton::LT));
	itemNext = (ControlButton)api->SetupIniInt(L"InputControls", L"ItemNext", static_cast<int>(ControlButton::RT));
	stepLeft = (ControlButton)api->SetupIniInt(L"InputControls", L"StepLeft", static_cast<int>(ControlButton::LB));
	stepRight = (ControlButton)api->SetupIniInt(L"InputControls", L"StepRight", static_cast<int>(ControlButton::RB));
	pause = (ControlButton)api->SetupIniInt(L"InputControls", L"Pause", static_cast<int>(ControlButton::Start));
}

} // namespace Input::Controls
