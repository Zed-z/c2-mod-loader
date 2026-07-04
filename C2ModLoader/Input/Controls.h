#pragma once

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
};

extern ControlAnalog movement;
extern ControlAnalog camera;
extern ControlButton jump;
extern ControlButton attack;
extern ControlButton cameraFlip;
extern ControlButton itemUse;
extern ControlButton itemPrev;
extern ControlButton itemNext;
extern ControlButton stepLeft;
extern ControlButton stepRight;
extern ControlButton pause;

void Setup();

} // namespace Input::Controls
