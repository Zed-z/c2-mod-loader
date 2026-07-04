#include "Input.h"

#include "Input/AnalogInput.h"
#include "Input/DpadMovement.h"
#include "Input/MidAirTurning.h"
#include "Input/TypeSwitching.h"
#include "Input/Vibration.h"

#include "Input/Backends/SDL3.h"
#include "Input/Backends/Xinput.h"

#include "ModApi.h"

extern ModApi *api;

namespace Input {

bool enabled;
int backend;

IInputBackend *inputBackend = nullptr;

ModernInput GetState() {
	ModernInput input = inputBackend->GetState();
	return input;
}

void __stdcall PollInput() {
	inputBackend->PollInput();
}

void Setup() {
	enabled = api->SetupIniBool(L"Input", L"Enabled", true);
	backend = api->SetupIniInt(L"Input", L"Backend", 0);

	if (!enabled)
		return;

	switch (backend) {
	case 0: // XInput
		inputBackend = new Input::Backends::Xinput::Backend();
		api->LogInfo("Using XInput backend for input.");
		break;
	case 1: // SDL3
		inputBackend = new Input::Backends::SDL3::Backend();
		api->LogInfo("Using SDL3 backend for input.");
		break;
	default:
		inputBackend = new Input::Backends::Xinput::Backend();
		api->LogInfo("Using XInput backend for input as fallback.");
		break;
	}

	inputBackend->Setup();
	api->HookGame(GAME_HOOK_PRE_INPUT, &PollInput);

	AnalogInput::Setup();
	Vibration::Setup();
	TypeSwitching::Setup();
	DpadMovement::Setup();
	MidAirTurning::Setup();
}

} // namespace Input
