#pragma once

#include "ModApi.h"

#include <vector>
#include <windows.h>

namespace MenuActions {

struct MenuAction {
	HMODULE handle;
	MenuActionRegistrationFunction function;
};

extern std::vector<MenuAction> menuActionRegistrations;

bool RegisterMenuAction(HMODULE handle, MenuActionRegistrationFunction registration);

} // namespace MenuActions
