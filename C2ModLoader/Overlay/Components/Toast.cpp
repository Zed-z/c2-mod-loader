#include "Toast.h"

#include "Logs.h"
#include "Overlay/Fonts.h"
#include "imgui.h"

#include <deque>
#include <string>

bool showToastInfo;
bool showToastDebug;
bool showToastWarning;
bool showToastError;

std::deque<Toast> toastQueue;

void ImGuiShowToast(const std::string &message, const LogSeverity &severity, float duration) {
	toastQueue.push_front({message, severity, duration});
}

void RenderToasts() {

	ImGuiIO &io = ImGui::GetIO();

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

	for (size_t i = 0; i < toastQueue.size();) {
		Toast &toast = toastQueue[i];
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
			ImGui::PushFont(Fonts::GetFontText(), fontSize);
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
			toastQueue.erase(toastQueue.begin() + i);
		} else {
			i++;
		}
	}
}
