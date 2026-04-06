#pragma once

#include "ModApi.h"

#include <windows.h>
#include <vector>

struct MenuAction {
	HMODULE handle;
	MenuActionRegistrationFunction function;
};

extern std::vector<MenuAction> menuActionRegistrations;

bool ImGuiRegisterMenuAction(HMODULE handle, MenuActionRegistrationFunction registration);

void RenderMenuBar();
