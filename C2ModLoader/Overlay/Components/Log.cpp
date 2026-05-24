#include "Log.h"

#include "Logs.h"
#include "ModApi.h"
#include "Utils/Fonts.h"
#include "imgui.h"

extern ModApi *api;

namespace Overlay::Log {

bool showLog;
bool showLogInfo;
bool showLogDebug;
bool showLogWarning;
bool showLogError;

void Setup() {
	showLog = api->SetupIniBool(L"GUI", L"ShowLog", false);
	showLogInfo = api->SetupIniBool(L"Logging", L"Info", true);
	showLogDebug = api->SetupIniBool(L"Logging", L"Debug", false);
	showLogWarning = api->SetupIniBool(L"Logging", L"Warning", true);
	showLogError = api->SetupIniBool(L"Logging", L"Error", true);
}

void RenderLog() {
	if (!showLog)
		return;

	bool prevShow = showLog;

	ImGui::Begin("Console Log", &showLog);

	if (ImGui::Checkbox("Show Info##log_showinfo", &showLogInfo)) {
		api->WriteIniBool(L"Logging", L"Info", showLogInfo);
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Show Debug##log_showdebug", &showLogDebug)) {
		api->WriteIniBool(L"Logging", L"Debug", showLogDebug);
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Show Warnings##log_showwarning", &showLogWarning)) {
		api->WriteIniBool(L"Logging", L"Warning", showLogWarning);
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Show Errors##log_showerror", &showLogError)) {
		api->WriteIniBool(L"Logging", L"Error", showLogError);
	}

	ImGui::PushFont(Fonts::GetFontCode());
	ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	bool autoScroll = false;
	float scrollY = ImGui::GetScrollY();
	float scrollMaxY = ImGui::GetScrollMaxY();

	if (scrollY >= scrollMaxY - 1.0f)
		autoScroll = true;

	std::vector<LogMessage> logSnapshot;
	{
		std::lock_guard<std::mutex> lock(logMutex);
		logSnapshot = logMessages;
	}

	for (const LogMessage &msg : logSnapshot) {

		if (msg.severity == LogSeverity::Info && !showLogInfo)
			continue;
		if (msg.severity == LogSeverity::Debug && !showLogDebug)
			continue;
		if (msg.severity == LogSeverity::Warning && !showLogWarning)
			continue;
		if (msg.severity == LogSeverity::Error && !showLogError)
			continue;

		ImVec4 logColor;
		switch (msg.severity) {
		case LogSeverity::Info:
			logColor = ImVec4(1, 1, 1, 1);
			break;
		case LogSeverity::Debug:
			logColor = ImVec4(0.5f, 0.8f, 1, 1);
			break;
		case LogSeverity::Warning:
			logColor = ImVec4(1, 1, 0.3f, 1);
			break;
		case LogSeverity::Error:
			logColor = ImVec4(1, 0.3f, 0.3f, 1);
			break;
		}
		ImGui::PushStyleColor(ImGuiCol_Text, logColor);
		ImGui::TextUnformatted(msg.text.c_str());
		ImGui::PopStyleColor();
	}

	if (autoScroll)
		ImGui::SetScrollHereY(1.0f);

	ImGui::EndChild();
	ImGui::PopFont();
	ImGui::End();

	if (prevShow != showLog) {
		api->WriteIniBool(L"GUI", L"ShowLog", showLog);
	}
}

} // namespace Overlay::Log
