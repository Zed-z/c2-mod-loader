#include "Win32.h"

#include "Input/Input.h"

#include <string>
#include <windows.h>

#include "ModApi.h"

extern ModApi *api;

namespace Input::Backends::Win32 {

bool Backend::getKey(const std::string &keyName) {
	if (keyName == "SPACE")
		return (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
	if (keyName == "CONTROL")
		return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
	if (keyName == "SHIFT")
		return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
	if (keyName == "ALT")
		return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
	if (keyName == "ENTER")
		return (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
	if (keyName == "ESCAPE")
		return (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
	if (keyName == "TAB")
		return (GetAsyncKeyState(VK_TAB) & 0x8000) != 0;
	if (keyName == "BACKSPACE")
		return (GetAsyncKeyState(VK_BACK) & 0x8000) != 0;
	if (keyName == "DELETE")
		return (GetAsyncKeyState(VK_DELETE) & 0x8000) != 0;
	if (keyName == "INSERT")
		return (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
	if (keyName == "HOME")
		return (GetAsyncKeyState(VK_HOME) & 0x8000) != 0;
	if (keyName == "END")
		return (GetAsyncKeyState(VK_END) & 0x8000) != 0;
	if (keyName == "PAGEUP")
		return (GetAsyncKeyState(VK_PRIOR) & 0x8000) != 0;
	if (keyName == "PAGEDOWN")
		return (GetAsyncKeyState(VK_NEXT) & 0x8000) != 0;
	if (keyName == "LEFT")
		return (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
	if (keyName == "RIGHT")
		return (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;
	if (keyName == "UP")
		return (GetAsyncKeyState(VK_UP) & 0x8000) != 0;
	if (keyName == "DOWN")
		return (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;
	if (keyName == "F1")
		return (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
	if (keyName == "F2")
		return (GetAsyncKeyState(VK_F2) & 0x8000) != 0;

	if (keyName == "NUMPAD0")
		return (GetAsyncKeyState(VK_NUMPAD0) & 0x8000) != 0;
	if (keyName == "NUMPAD1")
		return (GetAsyncKeyState(VK_NUMPAD1) & 0x8000) != 0;
	if (keyName == "NUMPAD2")
		return (GetAsyncKeyState(VK_NUMPAD2) & 0x8000) != 0;
	if (keyName == "NUMPAD3")
		return (GetAsyncKeyState(VK_NUMPAD3) & 0x8000) != 0;
	if (keyName == "NUMPAD4")
		return (GetAsyncKeyState(VK_NUMPAD4) & 0x8000) != 0;
	if (keyName == "NUMPAD5")
		return (GetAsyncKeyState(VK_NUMPAD5) & 0x8000) != 0;
	if (keyName == "NUMPAD6")
		return (GetAsyncKeyState(VK_NUMPAD6) & 0x8000) != 0;
	if (keyName == "NUMPAD7")
		return (GetAsyncKeyState(VK_NUMPAD7) & 0x8000) != 0;
	if (keyName == "NUMPAD8")
		return (GetAsyncKeyState(VK_NUMPAD8) & 0x8000) != 0;
	if (keyName == "NUMPAD9")
		return (GetAsyncKeyState(VK_NUMPAD9) & 0x8000) != 0;

	if (keyName == "NUMLOCK")
		return (GetAsyncKeyState(VK_NUMLOCK) & 0x8000) != 0;
	if (keyName == "SCROLLLOCK")
		return (GetAsyncKeyState(VK_SCROLL) & 0x8000) != 0;
	if (keyName == "CAPSLOCK")
		return (GetAsyncKeyState(VK_CAPITAL) & 0x8000) != 0;

	if (keyName == "PAUSE")
		return (GetAsyncKeyState(VK_PAUSE) & 0x8000) != 0;
	if (keyName == "PRINTSCREEN")
		return (GetAsyncKeyState(VK_SNAPSHOT) & 0x8000) != 0;
	if (keyName == "WINDOWS")
		return (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 || (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;
	if (keyName == "PAGEUP")
		return (GetAsyncKeyState(VK_PRIOR) & 0x8000) != 0;
	if (keyName == "PAGEDOWN")
		return (GetAsyncKeyState(VK_NEXT) & 0x8000) != 0;

	// Other keys
	char key = keyName[0];
	if (key >= 'A' && key <= 'Z')
		return (GetAsyncKeyState(key) & 0x8000) != 0;
	if (key >= '0' && key <= '9')
		return (GetAsyncKeyState(key) & 0x8000) != 0;

	return false;
}

void Backend::Setup() {
}

} // namespace Input::Backends::Win32
