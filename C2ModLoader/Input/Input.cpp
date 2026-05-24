#include "Input.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <windows.h>
#include <xinput.h>
#pragma comment(lib, "xinput.lib")

#include "Input/AnalogInput.h"
#include "Input/DpadMovement.h"
#include "Input/MidAirTurning.h"
#include "Input/TypeSwitching.h"
#include "Input/Vibration.h"

#include "ModApi.h"

extern ModApi *api;

namespace {

float stickDeadzone;
float stickOuterDeadzone;
float triggerDeadzone;
float triggerOuterDeadzone;

const float stickScale = 128.0f;
const float triggerScale = 128.0f;

using std::min, std::max;

template <typename T>
int sign(T val) {
	return (T(0) < val) - (val < T(0));
}

float applyStickDeadzone(float rawInput, float inner, float outer) {
	float absVal = std::abs(rawInput);
	if (absVal <= inner)
		return 0.0f;
	if (absVal >= outer)
		return sign(rawInput) * 1.0f;
	float normalized = (absVal - inner) / (outer - inner);
	return sign(rawInput) * normalized;
}

float applyTriggerDeadzone(float rawInput, float inner, float outer) {
	if (rawInput <= inner)
		return 0.0f;
	if (rawInput >= outer)
		return 1.0f;
	float normalized = (rawInput - inner) / (outer - inner);
	return normalized;
}

} // namespace

namespace Input {

bool enabled;
int deviceIndex;

ModernInput input{};

void __stdcall PollInput() {
	XINPUT_STATE state{};
	if (XInputGetState(Input::deviceIndex, &state) == ERROR_SUCCESS) {
		short rawLeftX = state.Gamepad.sThumbLX;
		short rawLeftY = state.Gamepad.sThumbLY;
		short rawRightX = state.Gamepad.sThumbRX;
		short rawRightY = state.Gamepad.sThumbRY;
		short rawLeftTrigger = state.Gamepad.bLeftTrigger;
		short rawRightTrigger = state.Gamepad.bRightTrigger;

		float leftX = rawLeftX / 32768.0f;
		float leftY = rawLeftY / 32768.0f;
		float rightX = rawRightX / 32768.0f;
		float rightY = rawRightY / 32768.0f;
		float leftTrigger = rawLeftTrigger / 255.0f;
		float rightTrigger = rawRightTrigger / 255.0f;

		input.leftStick.x = -applyStickDeadzone(leftX, stickDeadzone, stickOuterDeadzone) * stickScale;
		input.leftStick.y = applyStickDeadzone(leftY, stickDeadzone, stickOuterDeadzone) * stickScale;
		input.leftStick.click = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;

		input.rightStick.x = applyStickDeadzone(rightX, stickDeadzone, stickOuterDeadzone) * stickScale;
		input.rightStick.y = -applyStickDeadzone(rightY, stickDeadzone, stickOuterDeadzone) * stickScale;
		input.rightStick.click = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;

		input.leftTrigger = applyTriggerDeadzone(leftTrigger, triggerDeadzone, triggerOuterDeadzone) * triggerScale;
		input.rightTrigger = applyTriggerDeadzone(rightTrigger, triggerDeadzone, triggerOuterDeadzone) * triggerScale;

		input.dpad.up = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
		input.dpad.down = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
		input.dpad.left = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
		input.dpad.right = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;

		input.leftShoulder = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
		input.rightShoulder = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;

		input.aButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
		input.bButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
		input.xButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
		input.yButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;

		input.startButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
		input.backButton = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
	} else {
		input = {};
	}
	input.config.enabled = Input::enabled;
	input.config.stickScale = stickScale;
	input.config.triggerScale = triggerScale;
}

ModernInput GetState() {
	return input;
}

void Setup() {
	enabled = api->SetupIniBool(L"Input", L"Enabled", true);
	deviceIndex = api->SetupIniInt(L"Input", L"DeviceIndex", 0);

	stickDeadzone = api->SetupIniInt(L"Input", L"StickDeadzone", 25) / 100.0f;
	stickDeadzone = min(max(stickDeadzone, 0.0f), 1.0f);
	stickOuterDeadzone = api->SetupIniInt(L"Input", L"StickOuterDeadzone", 75) / 100.0f;
	stickOuterDeadzone = min(max(stickOuterDeadzone, 0.0f), 1.0f);
	triggerDeadzone = api->SetupIniInt(L"Input", L"TriggerDeadzone", 10) / 100.0f;
	triggerDeadzone = min(max(triggerDeadzone, 0.0f), 1.0f);
	triggerOuterDeadzone = api->SetupIniInt(L"Input", L"TriggerOuterDeadzone", 90) / 100.0f;
	triggerOuterDeadzone = min(max(triggerOuterDeadzone, 0.0f), 1.0f);

	if (!enabled)
		return;

	PollInput();
	api->HookGame(GAME_HOOK_PRE_INPUT, &PollInput);

	AnalogInput::Setup();
	Vibration::Setup();
	TypeSwitching::Setup();
	DpadMovement::Setup();
	MidAirTurning::Setup();
}

} // namespace Input
