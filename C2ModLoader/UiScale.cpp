#include "UiScale.h"

#include "ModApi.h"

#include "backends/imgui_impl_win32.h"

extern ModApi *api;

namespace {

float ReadScaleOverride() {
	const float value = api->SetupIniFloat(L"Config", L"DisplayScale", 0.0f);
	return (value > 0.0f) ? value : 0.0f;
}

float ProbeOsScale(HWND hwnd) {
	float scale = 1.0f;
	if (hwnd != nullptr) {
		scale = ImGui_ImplWin32_GetDpiScaleForHwnd(hwnd);
	} else {
		scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
	}
	if (scale <= 0.0f) {
		scale = 1.0f;
	}
	return scale;
}

} // namespace

namespace UiScale {

void EnableDpiAwareness() {
	ImGui_ImplWin32_EnableDpiAwareness();
}

float ResolveUiScale(HWND hwnd) {
	const float overrideScale = ReadScaleOverride();
	if (overrideScale > 0.0f) {
		return overrideScale;
	}
	return ProbeOsScale(hwnd);
}

} // namespace UiScale
