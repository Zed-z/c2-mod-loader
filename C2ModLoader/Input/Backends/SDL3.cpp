#include "SDL3.h"

#include "Input/Input.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <string>

#include "ModApi.h"

extern ModApi *api;

namespace {

SDL_Gamepad *activeGamepad = nullptr;
SDL_Gamepad *lastActiveGamepad = nullptr;
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

void RefreshGamepadConnection() {
	if (activeGamepad)
		return;

	int count = 0;
	SDL_JoystickID *joysticks = SDL_GetGamepads(&count);
	if (joysticks && count > 0) {
		int targetIndex = min(max(deviceIndex, 0), count - 1);
		activeGamepad = SDL_OpenGamepad(joysticks[targetIndex]);
		// api->LogDebug((std::string("Found ") + std::to_string(count) + " gamepad(s), opening index " + std::to_string(targetIndex)).c_str());
	} else {
		// api->LogDebug((std::string("No gamepads found. Count: ") + std::to_string(count)).c_str());
	}
	SDL_free(joysticks);

	if (activeGamepad) {
		if (SDL_GamepadHasSensor(activeGamepad, SDL_SENSOR_GYRO)) {
			SDL_SetGamepadSensorEnabled(activeGamepad, SDL_SENSOR_GYRO, true);
		}
	}
}

void vibrateImpl(int strength, int durationMs) {
	if (activeGamepad) {
		if (strength < 0)
			strength = 0;
		if (strength > 65535)
			strength = 65535;
		uint16_t vibrationStrength = static_cast<uint16_t>(strength);
		SDL_RumbleGamepad(activeGamepad, vibrationStrength, vibrationStrength, durationMs);
	}
}

} // namespace

namespace Input::Backends::SDL3 {

bool enabled;

void Backend::PollInput() {
	SDL_UpdateGamepads();
	RefreshGamepadConnection();

	if (activeGamepad != lastActiveGamepad) {
		if (activeGamepad != nullptr) {
			const char *name = SDL_GetGamepadName(activeGamepad);
			std::string msg = "Controller Connected: " + std::string(name ? name : "Unknown Controller");
			api->ShowInfoToast(msg.c_str());
		} else if (lastActiveGamepad != nullptr) {
			api->ShowInfoToast("Controller Disconnected");
		}
		lastActiveGamepad = activeGamepad;
	}

	input = {};

	if (activeGamepad && SDL_GamepadConnected(activeGamepad)) {
		float leftX = SDL_GetGamepadAxis(activeGamepad, SDL_GAMEPAD_AXIS_LEFTX) / 32768.0f;
		float leftY = SDL_GetGamepadAxis(activeGamepad, SDL_GAMEPAD_AXIS_LEFTY) / -32768.0f;
		float rightX = SDL_GetGamepadAxis(activeGamepad, SDL_GAMEPAD_AXIS_RIGHTX) / 32768.0f;
		float rightY = SDL_GetGamepadAxis(activeGamepad, SDL_GAMEPAD_AXIS_RIGHTY) / -32768.0f;

		float leftTrigger = SDL_GetGamepadAxis(activeGamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) / 32767.0f;
		float rightTrigger = SDL_GetGamepadAxis(activeGamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) / 32767.0f;

		input.leftStick.x = -applyStickDeadzone(leftX, stickDeadzone, stickOuterDeadzone) * stickScale;
		input.leftStick.y = applyStickDeadzone(leftY, stickDeadzone, stickOuterDeadzone) * stickScale;
		input.leftStick.click = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_LEFT_STICK);

		input.rightStick.x = -applyStickDeadzone(rightX, stickDeadzone, stickOuterDeadzone) * stickScale;
		input.rightStick.y = applyStickDeadzone(rightY, stickDeadzone, stickOuterDeadzone) * stickScale;
		input.rightStick.click = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_RIGHT_STICK);

		input.leftTrigger = applyTriggerDeadzone(leftTrigger, triggerDeadzone, triggerOuterDeadzone) * triggerScale;
		input.rightTrigger = applyTriggerDeadzone(rightTrigger, triggerDeadzone, triggerOuterDeadzone) * triggerScale;

		input.dpad.up = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_DPAD_UP);
		input.dpad.down = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
		input.dpad.left = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
		input.dpad.right = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);

		input.leftShoulder = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
		input.rightShoulder = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);

		input.aButton = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_SOUTH);
		input.bButton = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_EAST);
		input.xButton = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_WEST);
		input.yButton = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_NORTH);

		input.startButton = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_START);
		input.backButton = SDL_GetGamepadButton(activeGamepad, SDL_GAMEPAD_BUTTON_BACK);
	} else {
		if (activeGamepad) {
			SDL_CloseGamepad(activeGamepad);
			activeGamepad = nullptr;
		}
		input = {};
	}
	input.config.enabled = Input::enabled;
	input.config.stickScale = stickScale;
	input.config.triggerScale = triggerScale;

	if (doorStruct && *doorStruct) {
		ColorBGRA32 bg = (*doorStruct)->BackColor;
		SDL_SetGamepadLED(activeGamepad, bg.r, bg.g, bg.b);
		// api->LogDebug((std::string("Gamepad color: R=") + std::to_string(bg.r) + " G=" + std::to_string(bg.g) + " B=" + std::to_string(bg.b)).c_str());
	}
}

ModernInput Backend::GetState() {
	return input;
}

void Backend::Setup() {
	input = {};

	enabled = api->SetupIniBool(L"Input", L"Enabled", true);
	deviceIndex = api->SetupIniInt(L"Input", L"DeviceIndex", 0);

	api->LogInfo((std::string("SDL platform: ") + SDL_GetPlatform()).c_str());

	stickDeadzone = api->SetupIniInt(L"Input", L"StickDeadzone", 25) / 100.0f;
	stickDeadzone = min(max(stickDeadzone, 0.0f), 1.0f);
	stickOuterDeadzone = api->SetupIniInt(L"Input", L"StickOuterDeadzone", 75) / 100.0f;
	stickOuterDeadzone = min(max(stickOuterDeadzone, 0.0f), 1.0f);
	triggerDeadzone = api->SetupIniInt(L"Input", L"TriggerDeadzone", 10) / 100.0f;
	triggerDeadzone = min(max(triggerDeadzone, 0.0f), 1.0f);
	triggerOuterDeadzone = api->SetupIniInt(L"Input", L"TriggerOuterDeadzone", 90) / 100.0f;
	triggerOuterDeadzone = min(max(triggerOuterDeadzone, 0.0f), 1.0f);

	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_ENHANCED_REPORTS, "1");

	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
	SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_EVENTS);
}

void Backend::vibrate(int strength, int durationMs) {
	vibrateImpl(strength, durationMs);
}

} // namespace Input::Backends::SDL3
