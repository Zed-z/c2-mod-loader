#include "Controls.h"

#include <windows.h>

#include "Utils.h"

#include "ModApi.h"
extern ModApi *api;

namespace Input::Controls {

ControlsConfig config;

void Setup() {
	config.movement = (ControlAnalog)api->SetupIniInt(L"InputControls", L"Movement", static_cast<int>(ControlAnalog::LeftStick));
	config.camera = (ControlAnalog)api->SetupIniInt(L"InputControls", L"Camera", static_cast<int>(ControlAnalog::RightStick));
	config.jump = (ControlButton)api->SetupIniInt(L"InputControls", L"Jump", static_cast<int>(ControlButton::A));
	config.attack = (ControlButton)api->SetupIniInt(L"InputControls", L"Attack", static_cast<int>(ControlButton::B));
	config.cameraFlip = (ControlButton)api->SetupIniInt(L"InputControls", L"CameraFlip", static_cast<int>(ControlButton::X));
	config.itemUse = (ControlButton)api->SetupIniInt(L"InputControls", L"ItemUse", static_cast<int>(ControlButton::Y));
	config.itemPrev = (ControlButton)api->SetupIniInt(L"InputControls", L"ItemPrev", static_cast<int>(ControlButton::LT));
	config.itemNext = (ControlButton)api->SetupIniInt(L"InputControls", L"ItemNext", static_cast<int>(ControlButton::RT));
	config.stepLeft = (ControlButton)api->SetupIniInt(L"InputControls", L"StepLeft", static_cast<int>(ControlButton::LB));
	config.stepRight = (ControlButton)api->SetupIniInt(L"InputControls", L"StepRight", static_cast<int>(ControlButton::RB));
	config.pause = (ControlButton)api->SetupIniInt(L"InputControls", L"Pause", static_cast<int>(ControlButton::Start));

	constexpr DWORD keyLength = 256;
	wchar_t keyString[keyLength] = {};

	api->SetupIniString(L"InputControls", L"KeyMoveUp", L"", keyString, keyLength);
	config.keyMoveUp = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyMoveDown", L"", keyString, keyLength);
	config.keyMoveDown = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyMoveLeft", L"", keyString, keyLength);
	config.keyMoveLeft = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyMoveRight", L"", keyString, keyLength);
	config.keyMoveRight = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyCameraUp", L"", keyString, keyLength);
	config.keyCameraUp = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyCameraDown", L"", keyString, keyLength);
	config.keyCameraDown = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyCameraLeft", L"", keyString, keyLength);
	config.keyCameraLeft = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyCameraRight", L"", keyString, keyLength);
	config.keyCameraRight = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyJump", L"", keyString, keyLength);
	config.keyJump = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyAttack", L"", keyString, keyLength);
	config.keyAttack = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyCameraFlip", L"", keyString, keyLength);
	config.keyCameraFlip = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyItemUse", L"", keyString, keyLength);
	config.keyItemUse = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyItemPrev", L"", keyString, keyLength);
	config.keyItemPrev = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyItemNext", L"", keyString, keyLength);
	config.keyItemNext = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyStepLeft", L"", keyString, keyLength);
	config.keyStepLeft = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyStepRight", L"", keyString, keyLength);
	config.keyStepRight = WStringToString(keyString);
	api->SetupIniString(L"InputControls", L"KeyPause", L"", keyString, keyLength);
	config.keyPause = WStringToString(keyString);
}

} // namespace Input::Controls
