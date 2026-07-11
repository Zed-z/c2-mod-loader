#include "Input.h"

#include "Input/AirTurning.h"
#include "Input/AnalogInput.h"
#include "Input/Controls.h"
#include "Input/DpadMovement.h"
#include "Input/RemapActionButtons.h"
#include "Input/TypeSwitching.h"
#include "Input/Vibration.h"

#include "Input/Backends/SDL3.h"
#include "Input/Backends/Win32.h"
#include "Input/Backends/Xinput.h"

#include "ModApi.h"

extern ModApi *api;

namespace Input {

bool enabled;

IControllerBackend *controllerBackend = nullptr;
IKeyboardBackend *keyboardBackend = nullptr;

ModernInput GetState() {
	ModernInput input = controllerBackend->GetState();
	return input;
}

void __stdcall PollInput() {
	controllerBackend->PollInput();
}

void Setup() {
	enabled = api->SetupIniBool(L"Input", L"Enabled", true);
	int _controllerBackend = api->SetupIniInt(L"Input", L"ControllerBackend", 0);
	int _keyboardBackend = api->SetupIniInt(L"Input", L"KeyboardBackend", 0);

	if (!enabled)
		return;

	Controls::Setup();

	switch (_controllerBackend) {
	case 0: // XInput
		controllerBackend = new Input::Backends::Xinput::Backend();
		api->LogInfo("Using XInput backend for input.");
		break;
	case 1: // SDL3
		controllerBackend = new Input::Backends::SDL3::Backend();
		api->LogInfo("Using SDL3 backend for input.");
		break;
	default:
		controllerBackend = new Input::Backends::Xinput::Backend();
		api->LogInfo("Using XInput backend for input as fallback.");
		break;
	}

	switch (_keyboardBackend) {
	case 0: // Win32
		keyboardBackend = new Input::Backends::Win32::Backend();
		api->LogInfo("Using Win32 backend for keyboard input.");
		break;
	default:
		keyboardBackend = new Input::Backends::Win32::Backend();
		api->LogInfo("Using Win32 backend for keyboard input as fallback.");
		break;
	}

	controllerBackend->Setup();
	keyboardBackend->Setup();

	api->HookGame(GAME_HOOK_PRE_INPUT, &PollInput);

	AnalogInput::Setup();
	Vibration::Setup();
	TypeSwitching::Setup();
	DpadMovement::Setup();
	AirTurning::Setup();
	RemapActionButtons::Setup();
}

int getAnalogStickX(Input::Controls::ControlAnalog analog) {
	ModernInput input = Input::GetState();
	switch (analog) {
	case Input::Controls::ControlAnalog::LeftStick:
		return input.leftStick.x;
	case Input::Controls::ControlAnalog::RightStick:
		return input.rightStick.x;
	default:
		return 0;
	}
}

int getAnalogStickY(Input::Controls::ControlAnalog analog) {
	ModernInput input = Input::GetState();
	switch (analog) {
	case Input::Controls::ControlAnalog::LeftStick:
		return input.leftStick.y;
	case Input::Controls::ControlAnalog::RightStick:
		return input.rightStick.y;
	default:
		return 0;
	}
}

bool getButtonPressed(Input::Controls::ControlButton button) {
	ModernInput input = Input::GetState();
	switch (button) {
	case Input::Controls::ControlButton::A:
		return input.aButton;
	case Input::Controls::ControlButton::B:
		return input.bButton;
	case Input::Controls::ControlButton::X:
		return input.xButton;
	case Input::Controls::ControlButton::Y:
		return input.yButton;
	case Input::Controls::ControlButton::LB:
		return input.leftShoulder;
	case Input::Controls::ControlButton::RB:
		return input.rightShoulder;
	case Input::Controls::ControlButton::LT:
		return input.leftTrigger > 0.5f;
	case Input::Controls::ControlButton::RT:
		return input.rightTrigger > 0.5f;
	case Input::Controls::ControlButton::Start:
		return input.startButton;
	case Input::Controls::ControlButton::Select:
		return input.backButton;
	case Input::Controls::ControlButton::DpadUp:
		return input.dpad.up;
	case Input::Controls::ControlButton::DpadDown:
		return input.dpad.down;
	case Input::Controls::ControlButton::DpadLeft:
		return input.dpad.left;
	case Input::Controls::ControlButton::DpadRight:
		return input.dpad.right;
	case Input::Controls::ControlButton::LS:
		return input.leftStick.click;
	case Input::Controls::ControlButton::RS:
		return input.rightStick.click;
	default:
		return false;
	}
}

bool getKeyboardKeyPressed(const std::string &keyName) {
	return Input::keyboardBackend->getKey(keyName);
}

} // namespace Input
