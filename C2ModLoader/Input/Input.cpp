#include "Input.h"

#include "Input/AnalogInput.h"
#include "Input/Controls.h"
#include "Input/DpadMovement.h"
#include "Input/MidAirTurning.h"
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
	MidAirTurning::Setup();
	RemapActionButtons::Setup();
}

} // namespace Input
