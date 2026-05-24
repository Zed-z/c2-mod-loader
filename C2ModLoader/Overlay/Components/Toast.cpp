#include "Toast.h"

#include "Logs.h"
#include "Toasts.h"
#include "Utils/Fonts.h"
#include "imgui.h"

#include <deque>
#include <string>

#include "ModApi.h"
extern ModApi *api;

namespace Overlay::Toast {

bool showToastInfo;
bool showToastDebug;
bool showToastWarning;
bool showToastError;

void Setup() {
	showToastInfo = api->SetupIniBool(L"Toasts", L"Info", true);
	showToastDebug = api->SetupIniBool(L"Toasts", L"Debug", false);
	showToastWarning = api->SetupIniBool(L"Toasts", L"Warning", true);
	showToastError = api->SetupIniBool(L"Toasts", L"Error", true);
}

void RenderToasts() {

	ImGuiIO &io = ImGui::GetIO();
	const float uiScale = io.FontGlobalScale > 0.0f ? io.FontGlobalScale : 1.0f;

	float displayScale = io.DisplaySize.y / 720.0f;

	float toastWidth = 360.0f * displayScale;
	float toastMargin = 10.0f * displayScale;

	float margin = 20.0f * displayScale;
	float padding = 20.0f * displayScale;

	float originX = io.DisplaySize.x - toastWidth - margin * displayScale;
	float originY = margin * displayScale;

	float borderWidth = 2.0f * displayScale;
	float windowRounding = 8.0f * displayScale;

	float fontSize = 24.0f * displayScale;

	for (size_t i = 0; i < Toasts::toastQueue.size();) {
		Toasts::Toast &toast = Toasts::toastQueue[i];
		toast.timeRemaining -= io.DeltaTime;

		// Fade out in the last 0.5 seconds
		float alpha = 1.0f;
		if (toast.timeRemaining < 0.5f) {
			alpha = toast.timeRemaining / 0.5f;
		}

		if (
			toast.severity == LogSeverity::Info && showToastInfo || toast.severity == LogSeverity::Debug && showToastDebug || toast.severity == LogSeverity::Warning && showToastWarning || toast.severity == LogSeverity::Error && showToastError) {
			ImVec4 toastColor;
			switch (toast.severity) {
			case LogSeverity::Info:
				toastColor = ImVec4(1, 1, 1, alpha);
				break;
			case LogSeverity::Debug:
				toastColor = ImVec4(0.5f, 0.8f, 1, alpha);
				break;
			case LogSeverity::Warning:
				toastColor = ImVec4(1, 1, 0.3f, alpha);
				break;
			case LogSeverity::Error:
				toastColor = ImVec4(1, 0.3f, 0.3f, alpha);
				break;
			}

			ImGui::SetNextWindowBgAlpha(alpha * 0.85f);
			ImGui::SetNextWindowPos(ImVec2(originX, originY), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(toastWidth, 0), ImGuiCond_Always);

			ImGuiWindowFlags flags =
				ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, borderWidth);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, windowRounding);
			ImGui::PushStyleColor(ImGuiCol_Border, toastColor);
			ImGui::Begin(("##Toast" + std::to_string(i)).c_str(), nullptr, flags);

			// Text with wrapping
			ImGui::PushFont(Fonts::GetFontTitle(), fontSize / uiScale); // Neutralize UI scale
			ImGui::PushStyleColor(ImGuiCol_Text, toastColor);
			ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionMax().x);
			ImGui::TextUnformatted(toast.message.c_str());
			ImGui::PopTextWrapPos();
			ImGui::PopStyleColor();
			ImGui::PopFont();

			float toastHeight = ImGui::GetWindowSize().y;
			ImGui::End();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();

			// Add offset based on toast logHeight
			originY += toastHeight + toastMargin;
		}

		// Erase toast if needed
		if (toast.timeRemaining <= 0.0f) {
			Toasts::toastQueue.erase(Toasts::toastQueue.begin() + i);
		} else {
			i++;
		}
	}
}

} // namespace Overlay::Toast
