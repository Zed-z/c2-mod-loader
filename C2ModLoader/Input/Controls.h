#pragma once

#include <string>

namespace Input::Controls {

enum class ControlAnalog {
	LeftStick,
	RightStick,
};

enum class ControlButton {
	A,
	B,
	X,
	Y,
	Start,
	Select,
	LB,
	RB,
	LT,
	RT,
	LS,
	RS,
	DpadUp,
	DpadDown,
	DpadLeft,
	DpadRight,
};

struct ControlsConfig {
	ControlAnalog movement;
	ControlAnalog camera;
	ControlButton cameraZoom;
	ControlButton jump;
	ControlButton attack;
	ControlButton cameraFlip;
	ControlButton itemUse;
	ControlButton itemPrev;
	ControlButton itemNext;
	ControlButton stepLeft;
	ControlButton stepRight;
	ControlButton pause;
	std::string keyMoveUp;
	std::string keyMoveDown;
	std::string keyMoveLeft;
	std::string keyMoveRight;
	std::string keyCameraUp;
	std::string keyCameraDown;
	std::string keyCameraLeft;
	std::string keyCameraRight;
	std::string keyCameraZoom;
	std::string keyJump;
	std::string keyAttack;
	std::string keyCameraFlip;
	std::string keyItemUse;
	std::string keyItemPrev;
	std::string keyItemNext;
	std::string keyStepLeft;
	std::string keyStepRight;
	std::string keyPause;
};

extern ControlsConfig config;

void Setup();

} // namespace Input::Controls
