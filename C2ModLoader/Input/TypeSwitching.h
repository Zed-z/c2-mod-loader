#pragma once

#include "ModApi.h"

namespace Input::TypeSwitching {

enum TypeSwitchMode {
	None = 0,
	Manual = 1,
	Automatic = 2
};

extern TypeSwitchMode typeSwitchMode;

void __stdcall resetUsingKeyboard();

void __stdcall setUsingKeyboard();

void Setup();

} // namespace Input::TypeSwitching
