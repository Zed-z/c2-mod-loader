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

bool vibrationEnabled;
float vibrationStrength;

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
	input.config.enabled = xinputEnabled;
	input.config.stickScale = stickScale;
	input.config.triggerScale = triggerScale;
}

XinputInput GetState() {
	return input;
}

void __stdcall PreInput() {
	PollInput();
}

void PatchPreInput() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			Croc2.exe+2EEC0 - 83 EC 10              - sub esp,10
			Croc2.exe+2EEC3 - 8B 0D ACA55200        - mov ecx,[Croc2.exe+12A5AC]
		*/
		api->HookFunction(0x42EEC0, 9, &PreInput, INJECT_BEFORE);
		break;
	}
	case GAMEVER_EU: {
		api->LogWarning("[XinputManager] Not implemented.");
		break;
	}
	case GAMEVER_DEMO: {
		api->LogWarning("[XinputManager] Not implemented.");
		break;
	}
	}
}

void PatchAnalogInput() {
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

	/*
		Inject before these lines, to replace analog input after
		it's polled from directinput, but before further processing
	*/
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			Croc2.exe+2F1CA - 83 FD 7F              - cmp ebp,7F
			Croc2.exe+2F1CD - 89 15 74A55200        - mov [Croc2.exe+12A574],edx
		*/
		api->InjectCode(0x42F1CA, 9, hookCode, p, INJECT_BEFORE);
	}
	case GAMEVER_EU: {
		api->LogWarning("[XinputManager] Not implemented.");
		return;
	}
	case GAMEVER_DEMO: {
		api->LogWarning("[XinputManager] Not implemented.");
		return;
	}
	}
}

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
	XInputSetState(xinputDeviceIndex, &vibrationEnable);

	Sleep(durationMs);

	XINPUT_VIBRATION vibrationDisable{};
	vibrationDisable.wLeftMotorSpeed = 0;
	vibrationDisable.wRightMotorSpeed = 0;
	XInputSetState(xinputDeviceIndex, &vibrationDisable);

	return 0;
}

void vibrate(int strength, int durationMs) {
	VibrationParams *params = new VibrationParams{strength, durationMs};
	HANDLE threadHandle = CreateThread(nullptr, 0, vibrationThread, params, 0, nullptr);
	if (threadHandle != nullptr) {
		CloseHandle(threadHandle);
	}
}

void __stdcall PreDamage() {
	if (vibrationEnabled) {
		int strength = (int)(vibrationStrength * 65535);
		vibrate(strength, 200);
	}
}

void PatchPreDamage() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		// Croc2.exe+81B80 - 83 3D 3C794B00 0B     - cmp dword ptr [Croc2.exe+B793C],0B
		api->HookFunction(0x481B80, 7, &PreDamage, INJECT_BEFORE);
		break;
	}
	case GAMEVER_EU: {
		api->LogWarning("[XinputManager] Not implemented.");
		break;
	}
	case GAMEVER_DEMO: {
		api->LogWarning("[XinputManager] Not implemented.");
		break;
	}
	}
}

void Setup() {
	// Load config
	xinputEnabled = api->SetupIniBool(L"Xinput", L"XinputEnabled", true);
	xinputDeviceIndex = api->SetupIniInt(L"Xinput", L"DeviceIndex", 0);
	stickDeadzone = api->SetupIniInt(L"Xinput", L"StickDeadzone", 25) / 100.0f;
	stickDeadzone = min(max(stickDeadzone, 0.0f), 1.0f);
	stickOuterDeadzone = api->SetupIniInt(L"Xinput", L"StickOuterDeadzone", 75) / 100.0f;
	stickOuterDeadzone = min(max(stickOuterDeadzone, 0.0f), 1.0f);
	triggerDeadzone = api->SetupIniInt(L"Xinput", L"TriggerDeadzone", 10) / 100.0f;
	triggerDeadzone = min(max(triggerDeadzone, 0.0f), 1.0f);
	triggerOuterDeadzone = api->SetupIniInt(L"Xinput", L"TriggerOuterDeadzone", 90) / 100.0f;
	triggerDeadzone = min(max(triggerDeadzone, 0.0f), 1.0f);
	vibrationEnabled = api->SetupIniBool(L"Xinput", L"VibrationEnabled", true);
	vibrationStrength = api->SetupIniInt(L"Xinput", L"VibrationStrength", 100) / 100.0f;
	vibrationStrength = min(max(vibrationStrength, 0.0f), 1.0f);

	PollInput();
	if (!xinputEnabled)
		return;

	// Patch the game to use Xinput
	PatchPreInput();
	PatchAnalogInput();
	PatchPreDamage();
}

} // namespace XinputManager
