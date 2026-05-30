#pragma once

#include "ModApi.h"

namespace Overlay::ObjectList {

extern bool showObjectList;

void Setup();

void RenderObjectList();

void SelectObject(StratEntity *object);

} // namespace Overlay::ObjectList
