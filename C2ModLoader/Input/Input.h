#pragma once

#include "ModApi.h"

namespace Input {

extern bool enabled;

interface IInputBackend {
	ModernInput input;
	virtual void PollInput() = 0;
	virtual ModernInput GetState() = 0;
	virtual void Setup() = 0;
	virtual void vibrate(int strength, int durationMs) = 0;
};

extern IInputBackend *inputBackend;

ModernInput GetState();
void Setup();

} // namespace Input
