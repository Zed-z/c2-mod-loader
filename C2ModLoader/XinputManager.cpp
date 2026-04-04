#include "XinputManager.h"
#include "ModApi.h"
#include <cmath>
#include <string>
#include <windows.h>
#include <xinput.h>
#pragma comment(lib, "xinput.lib")

extern ModApi *api;

namespace {

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

namespace XinputManager {

bool xinputEnabled;
int xinputDeviceIndex;
float stickDeadzone;
float stickOuterDeadzone;
float triggerDeadzone;
float triggerOuterDeadzone;

const float stickScale = 128.0f;
const float triggerScale = 128.0f;

XinputInput input{};

void PollInput() {
	XINPUT_STATE state{};
	if (XInputGetState(xinputDeviceIndex, &state) == ERROR_SUCCESS) {
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
		input.rightStick.x = applyStickDeadzone(rightX, stickDeadzone, stickOuterDeadzone) * stickScale;
		input.rightStick.y = -applyStickDeadzone(rightY, stickDeadzone, stickOuterDeadzone) * stickScale;
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
	input.config.enabled = xinputEnabled;
	input.config.stickScale = stickScale;
	input.config.triggerScale = triggerScale;
}

XinputInput GetState() {
	return input;
}

/*
	Croc2.exe+2EEC0 - 83 EC 10              - sub esp,10
	Croc2.exe+2EEC3 - 8B 0D ACA55200        - mov ecx,[Croc2.exe+12A5AC]
*/
void __stdcall PreInput() {
	PollInput();
}

void PatchPreInput() {
	api->HookFunction(0x42EEC0, 9, &PreInput, INJECT_BEFORE);
}

void PatchAnalogInput() {
	/*
		Inject before these lines, to replace analog input after
		it's polled from directinput, but before further processing
		Croc2.exe+2F1CA - 83 FD 7F              - cmp ebp,7F
		Croc2.exe+2F1CD - 89 15 74A55200        - mov [Croc2.exe+12A574],edx
	*/
	uintptr_t hookAddress = 0x42F1CA;
	uintptr_t analogYAddress = 0x52A574;
	uintptr_t ptrX = (uintptr_t)&(input.leftStick.x);
	uintptr_t ptrY = (uintptr_t)&(input.leftStick.y);
	uint8_t hookCode[12];
	int p = 0;

	// mov ebp, [ptrX]
	hookCode[p++] = 0x8B;
	hookCode[p++] = 0x2D;
	*(uintptr_t *)(hookCode + p) = ptrX;
	p += 4;

	// mov edx, [ptrY]
	hookCode[p++] = 0x8B;
	hookCode[p++] = 0x15;
	*(uintptr_t *)(hookCode + p) = ptrY;
	p += 4;

	api->InjectCode(hookAddress, 9, hookCode, p, INJECT_BEFORE);
}

void Setup() {
	// Load config
	xinputEnabled = api->SetupIniBool(L"Xinput", L"XinputEnabled", true);
	xinputDeviceIndex = api->SetupIniInt(L"Xinput", L"DeviceIndex", 0);
	stickDeadzone = api->SetupIniInt(L"Xinput", L"StickDeadzone", 25) / 100.0f;
	stickOuterDeadzone = api->SetupIniInt(L"Xinput", L"StickOuterDeadzone", 75) / 100.0f;
	triggerDeadzone = api->SetupIniInt(L"Xinput", L"TriggerDeadzone", 10) / 100.0f;
	triggerOuterDeadzone = api->SetupIniInt(L"Xinput", L"TriggerOuterDeadzone", 90) / 100.0f;

	PollInput();
	if (!xinputEnabled)
		return;

	// Patch the game to use Xinput
	PatchPreInput();
	PatchAnalogInput();
}

} // namespace XinputManager
