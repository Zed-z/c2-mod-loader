#pragma once

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

ModernInput GetState();
void Setup();

} // namespace Input
