#pragma once

#include "ModApi.h"

namespace Input {

extern bool enabled;
extern int deviceIndex;

extern ModernInput input;

ModernInput GetState();

void Setup();

} // namespace Input
