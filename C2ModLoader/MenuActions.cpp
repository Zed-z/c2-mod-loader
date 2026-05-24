#include "MenuActions.h"

#include <vector>
#include <windows.h>

namespace MenuActions {

std::vector<MenuAction> menuActionRegistrations;

bool RegisterMenuAction(HMODULE handle, MenuActionRegistrationFunction registration) {
	menuActionRegistrations.push_back({handle, registration});
	return true;
}

} // namespace MenuActions
