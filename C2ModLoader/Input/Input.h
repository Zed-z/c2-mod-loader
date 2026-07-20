#pragma once

#include "Input/Controls.h"
#include "ModApi.h"

#include <string>

namespace Input {

extern bool enabled;

interface IControllerBackend {
	ModernInput input;
	virtual void PollInput() = 0;
	virtual ModernInput GetState() = 0;
	virtual void Setup() = 0;
	virtual void vibrate(int strength, int durationMs) = 0;
};

interface IKeyboardBackend {
	virtual void Setup() = 0;
	virtual bool getKey(const std::string &keyName) = 0;
};

extern IControllerBackend *controllerBackend;
extern IKeyboardBackend *keyboardBackend;

void __stdcall PollInput();
ModernInput GetState();
void Setup();

int getAnalogStickX(Input::Controls::ControlAnalog analog);
int getAnalogStickY(Input::Controls::ControlAnalog analog);
bool getButtonPressed(Input::Controls::ControlButton button);
bool getKeyboardKeyPressed(const std::string &keyName);

} // namespace Input
