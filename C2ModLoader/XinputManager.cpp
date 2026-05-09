#include "XinputManager.h"
#include "ModApi.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <windows.h>
#include <xinput.h>
#pragma comment(lib, "xinput.lib")

using std::min, std::max;

extern ModApi *api;

namespace {

enum TypeSwitchMode {
	None = 0,
	Manual = 1,
	Automatic = 2
};

std::string typeSwitchModeNames[] = {"None", "Manual", "Automatic"};
TypeSwitchMode typeSwitchMode;
bool dpadMovement;

int GetControlScheme() {
	SaveSlot *currentSaveSlot = api->GetCurrentSaveSlot();
	return currentSaveSlot->controlMethod;
}

void SetControlScheme(int scheme) {
	SaveSlot *currentSaveSlot = api->GetCurrentSaveSlot();
	currentSaveSlot->controlMethod = scheme;
}

void __stdcall resetUsingKeyboard() {
	if (typeSwitchMode == TypeSwitchMode::Automatic)
		SetControlScheme(CTRL_TYPE_1);
}

void __stdcall setUsingKeyboard() {
	if (typeSwitchMode == TypeSwitchMode::Automatic)
		SetControlScheme(CTRL_TYPE_2);
}

void PatchAutoModeSwitch() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			Croc2.exe+2F00A - 89 3D 4CA55200        - mov [Croc2.exe+12A54C],edi
		*/
		api->HookFunction(0x42f00a, 6, &resetUsingKeyboard, INJECT_AFTER);
		/*
			Croc2.exe+2F0F8 - 85 F6                 - test esi,esi
			Croc2.exe+2F0FA - 66 A3 8EA55200        - mov [Croc2.exe+12A58E],ax
			Croc2.exe+2F100 - 74 14                 - je Croc2.exe+2F116
			Croc2.exe+2F102 - C7 05 4CA55200 B5000000 - mov [Croc2.exe+12A54C],000000B5
			Croc2.exe+2F10C - C7 05 60A55200 02000000 - mov [Croc2.exe+12A560],00000002
		*/
		api->HookFunction(0x42f102, 10, &setUsingKeyboard, INJECT_AFTER);
		break;
	}
	case GAMEVER_EU: {
		/*
			Croc2.exe+2F78A - 89 3D 3C175300        - mov [Croc2.exe+13173C],edi
		*/
		api->HookFunction(0x42F78A, 6, &resetUsingKeyboard, INJECT_AFTER);
		/*
			Croc2.exe+2F878 - 85 F6                 - test esi,esi
			Croc2.exe+2F87A - 66 A3 7E175300        - mov [Croc2.exe+13177E],ax
			Croc2.exe+2F880 - 74 14                 - je Croc2.exe+2F896
			Croc2.exe+2F882 - C7 05 3C175300 B5000000 - mov [Croc2.exe+13173C],000000B5
			Croc2.exe+2F88C - C7 05 50175300 02000000 - mov [Croc2.exe+131750],00000002
		*/
		api->HookFunction(0x42F882, 10, &setUsingKeyboard, INJECT_AFTER);
		break;
	}
	case GAMEVER_DEMO: {
		/*
			Croc2.exe+2F43A - 89 3D 44955200        - mov [Croc2.exe+129544],edi
		*/
		api->HookFunction(0x42F43A, 6, &resetUsingKeyboard, INJECT_AFTER);
		/*
			Croc2.exe+2F528 - 85 F6                 - test esi,esi
			Croc2.exe+2F52A - 66 A3 86955200        - mov [Croc2.exe+129586],ax
			Croc2.exe+2F530 - 74 14                 - je Croc2.exe+2F546
			Croc2.exe+2F532 - C7 05 44955200 B5000000 - mov [Croc2.exe+129544],000000B5
			Croc2.exe+2F53C - C7 05 58955200 02000000 - mov [Croc2.exe+129558],00000002
		*/
		api->HookFunction(0x42F532, 10, &setUsingKeyboard, INJECT_AFTER);
		break;
	}
	}
}

void ManualModeSwitch() {
	if (typeSwitchMode == TypeSwitchMode::Manual) {
		bool changeControls = GetAsyncKeyState(VK_CAPITAL) & 1;
		if (changeControls) {
			int controlScheme = GetControlScheme();
			SetControlScheme(!controlScheme);
			api->ShowInfoToast(("Control scheme: " + std::string(ControlSchemeNames[!controlScheme])).c_str());
		}
	}
}

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
	ManualModeSwitch();
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
		/*
			Croc2.exe+2F640 - 83 EC 10              - sub esp,10
			Croc2.exe+2F643 - 8B 0D 9C175300        - mov ecx,[Croc2.exe+13179C]
		*/
		api->HookFunction(0x42F640, 9, &PreInput, INJECT_BEFORE);
		break;
	}
	case GAMEVER_DEMO: {
		/*
			Croc2.exe+2F2F0 - 83 EC 10              - sub esp,10
			Croc2.exe+2F2F3 - 8B 0D A4955200        - mov ecx,[Croc2.exe+1295A4]
		*/
		api->HookFunction(0x42F2F0, 9, &PreInput, INJECT_BEFORE);
		break;
	}
	}
}

void __stdcall doDpadMovement() {
	if (!dpadMovement)
		return;

	if (input.dpad.up)
		*analogY = 8128;
	if (input.dpad.down)
		*analogY = -8128;
	if (input.dpad.left)
		*analogX = 8128;
	if (input.dpad.right)
		*analogX = -8128;

	if (input.dpad.up || input.dpad.down || input.dpad.left || input.dpad.right) {
		*analogStrength = 181;
		*inputDeviceType = 2;
		setUsingKeyboard();
	}
}

void PatchDpadMovement() {
	switch (api->GetGameVersion()) {
	case GAMEVER_US: {
		/*
			Croc2.exe+2F0D1 - DB 05 64A55200        - fild dword ptr [Croc2.exe+12A564]
		*/
		api->HookFunction(0x42F0D1, 6, &doDpadMovement, INJECT_BEFORE);
		break;
	}
	case GAMEVER_EU: {
		/*
			Croc2.exe+2F851 - DB 05 54175300        - fild dword ptr [Croc2.exe+131754]
		*/
		api->HookFunction(0x42F851, 6, &doDpadMovement, INJECT_BEFORE);
		break;
	}
	case GAMEVER_DEMO: {
		/*
			Croc2.exe+2F501 - DB 05 5C955200        - fild dword ptr [Croc2.exe+12955C]
		*/
		api->HookFunction(0x42F501, 6, &doDpadMovement, INJECT_BEFORE);
		return;
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
		break;
	}
	case GAMEVER_EU: {
		/*
			Croc2.exe+2F94A - 83 FD 7F              - cmp ebp,7F
			Croc2.exe+2F94D - 89 15 64175300        - mov [Croc2.exe+131764],edx
		*/
		api->InjectCode(0x42F94A, 9, hookCode, p, INJECT_BEFORE);
		break;
	}
	case GAMEVER_DEMO: {
		/*
			Croc2.exe+2F5FA - 83 FD 7F              - cmp ebp,7F
			Croc2.exe+2F5FD - 89 15 6C955200        - mov [Croc2.exe+12956C],edx
		*/
		api->InjectCode(0x42F5FA, 9, hookCode, p, INJECT_BEFORE);
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
		// Croc2.exe+826D0 - 83 3D 2CEB4B00 0B     - cmp dword ptr [Croc2.exe+BEB2C],0B
		api->HookFunction(0x4826D0, 7, &PreDamage, INJECT_BEFORE);
		break;
	}
	case GAMEVER_DEMO: {
		// Croc2.exe+81010 - 83 3D 3C694B00 0B     - cmp dword ptr [Croc2.exe+B693C],0B
		api->HookFunction(0x481010, 7, &PreDamage, INJECT_BEFORE);
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
	typeSwitchMode = (TypeSwitchMode)api->SetupIniInt(L"Xinput", L"TypeSwitchMode", TypeSwitchMode::None);
	dpadMovement = api->SetupIniBool(L"Xinput", L"DpadMovement", true);

	PollInput();
	if (!xinputEnabled)
		return;

	// Patch the game to use Xinput
	PatchPreInput();
	PatchAnalogInput();
	PatchPreDamage();
	PatchAutoModeSwitch();
	PatchDpadMovement();
}

} // namespace XinputManager
