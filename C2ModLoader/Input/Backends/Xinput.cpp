#include "Xinput.h"

#include "Input/Input.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <windows.h>
#include <xinput.h>
#pragma comment(lib, "xinput.lib")

#include "ModApi.h"

extern ModApi *api;

namespace {

int deviceIndex;

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

namespace {

struct VibrationParams {
	int strength;
	int durationMs;
};

static DWORD WINAPI vibrationThread(LPVOID param) {
	VibrationParams *vibParams = (VibrationParams *)param;
	int strength = vibParams->strength;
	int durationMs = vibParams->durationMs;
	delete vibParams;

	XINPUT_VIBRATION vibrationEnable{};
	vibrationEnable.wLeftMotorSpeed = strength;
	vibrationEnable.wRightMotorSpeed = strength;
	XInputSetState(deviceIndex, &vibrationEnable);

	Sleep(durationMs);

	XINPUT_VIBRATION vibrationDisable{};
	vibrationDisable.wLeftMotorSpeed = 0;
	vibrationDisable.wRightMotorSpeed = 0;
	XInputSetState(deviceIndex, &vibrationDisable);

	return 0;
}

void vibrateImpl(int strength, int durationMs) {
	if (strength < 0)
		strength = 0;
	if (strength > 65535)
		strength = 65535;
	VibrationParams *params = new VibrationParams{strength, durationMs};
	HANDLE threadHandle = CreateThread(nullptr, 0, vibrationThread, params, 0, nullptr);
	if (threadHandle != nullptr) {
		CloseHandle(threadHandle);
	} else {
		delete params;
	}
}

} // namespace

namespace Input::Backends::Xinput {

bool enabled;

void Backend::PollInput() {
	XINPUT_STATE state{};
	input = {};
	if (XInputGetState(deviceIndex, &state) == ERROR_SUCCESS) {
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

		input.rightStick.x = -applyStickDeadzone(rightX, stickDeadzone, stickOuterDeadzone) * stickScale;
		input.rightStick.y = applyStickDeadzone(rightY, stickDeadzone, stickOuterDeadzone) * stickScale;
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
	input.config.enabled = enabled;
	input.config.stickScale = stickScale;
	input.config.triggerScale = triggerScale;
}

ModernInput Backend::GetState() {
	return input;
}

void Backend::Setup() {
	input = {};

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
}

void Backend::vibrate(int strength, int durationMs) {
	vibrateImpl(strength, durationMs);
}

} // namespace Input::Backends::Xinput
