#pragma once

#include <windows.h>

struct ModApi;

namespace UiScale {

void EnableDpiAwareness();
float ResolveUiScale(HWND hwnd = nullptr);

} // namespace UiScale
