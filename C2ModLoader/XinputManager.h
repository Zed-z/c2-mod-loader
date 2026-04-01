#pragma once

#include "ModApi.h"

namespace XinputManager {

extern bool xinputEnabled;
extern float stickDeadzone;
extern float triggerDeadzone;

XinputInput GetState();

void Setup();

} // namespace XinputManager
