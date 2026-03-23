#include "Launcher.h"
#include "Backend.h"
#include "Config.h"
#include "EmbeddedLicenses.h"
#include "IniConfig.h"
#include "Loader.h"
#include "ModApi.h"
#include "Resource.h"
#include "Utils.h"

#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"

#include <d3d11.h>
#include <map>
#include <shellapi.h>
#include <sstream>
#include <string>
#include <vector>
#pragma comment(lib, "shell32.lib")

extern ModApi *api;

int minWindowWidth = 720;
int minWindowHeight = 480;

extern bool loaderEnabled;

namespace {
using ConfigEntry = LauncherIni::ConfigEntry;

struct ConfigSectionView {
	std::string name;
	std::vector<size_t> entryIndices;
};

struct ConfigHint {
	std::string type;
	std::string defaultValue;
	std::vector<std::string> options; // Enum options for dropdowns
};

struct ParsedConfigHints {
	std::vector<std::pair<std::string, ConfigHint>> ordered;
	std::map<std::string, ConfigHint> lookup;
};

// 0 = loader, 1+ = mods
int g_selectedMod = 0;
std::vector<std::vector<ConfigEntry>> g_allConfigs;
std::vector<std::map<std::string, ConfigHint>> g_allHintMaps;

std::vector<Launcher::EmbeddedLicense> licenses = Launcher::GetEmbeddedLicenses();

std::wstring GetConfigPath(int modIndex) {
	if (modIndex == 0) { // TODO: possibly overkill
		wchar_t exePath[MAX_PATH] = {};
		GetModuleFileNameW(GetModuleHandleA(NULL), exePath, MAX_PATH);
		PathInfo exeInfo = GetPathInfo(std::wstring(exePath));
		return exeInfo.directory + L"\\" + CONFIG_FILE_L;
	}
	std::wstring path = mods[modIndex - 1].path.path;
	path.replace(path.length() - 4, 4, L".ini");
	return path;
}

void SaveConfigForMod(int modIndex) {
	LauncherIni::WriteIniFile(GetConfigPath(modIndex), g_allConfigs[modIndex]);
}

ParsedConfigHints ParseConfigHints(const std::wstring &configTypes) {
	ParsedConfigHints parsed;
	if (configTypes.empty())
		return parsed;

	std::string str = WStringToString(configTypes);
	std::istringstream ss(str);
	std::string token;

	while (std::getline(ss, token, ',')) {
		// Expected format: "Section/Key:type=default"
		size_t colonPos = token.find(':');
		if (colonPos == std::string::npos)
			continue;
		size_t equalsPos = token.find('=', colonPos + 1);
		if (equalsPos == std::string::npos)
			continue;

		const std::string keyPath = token.substr(0, colonPos);
		ConfigHint hint;
		hint.defaultValue = token.substr(equalsPos + 1);
		hint.type = token.substr(colonPos + 1, equalsPos - colonPos - 1);
		if (hint.type == "bool" && (hint.defaultValue == "true" || hint.defaultValue == "True"))
			hint.defaultValue = "1";
		if (hint.type.find("enum") == 0) {
			std::istringstream ss(hint.type);
			std::string token;
			size_t openBracket = hint.type.find('[');
			if (openBracket == std::string::npos)
				continue;
			size_t closeBracket = hint.type.find(']');
			if (closeBracket == std::string::npos || closeBracket < openBracket)
				continue;
			const std::string optionsStr = hint.type.substr(openBracket + 1, closeBracket - openBracket - 1);
			std::istringstream optionsSS(optionsStr);
			while (std::getline(optionsSS, token, '|')) {
				hint.options.push_back(token);
			}
			hint.type = "enum";
		}

		parsed.ordered.push_back({keyPath, hint});
		parsed.lookup[keyPath] = hint;
	}
	return parsed;
}

bool EnsureConfigDefaults(std::vector<ConfigEntry> &config, const std::vector<std::pair<std::string, ConfigHint>> &orderedHints) {
	bool changed = false;

	for (const auto &[keyPath, hint] : orderedHints) {
		size_t slashPos = keyPath.find('/');
		if (slashPos == std::string::npos || slashPos == 0 || slashPos + 1 >= keyPath.size()) {
			continue;
		}

		const std::string section = keyPath.substr(0, slashPos);
		const std::string key = keyPath.substr(slashPos + 1);

		bool exists = false;
		for (const auto &entry : config) {
			if (_stricmp(entry.section.c_str(), section.c_str()) == 0 &&
				_stricmp(entry.key.c_str(), key.c_str()) == 0) {
				exists = true;
				break;
			}
		}

		if (!exists) {
			config.push_back({section, key, hint.defaultValue});
			changed = true;
		}
	}

	return changed;
}

std::string NormalizeConfigValue(std::string value, const std::string &type) {
	if (type == "bool") {
		if (_stricmp(value.c_str(), "true") == 0)
			return "1";
		if (_stricmp(value.c_str(), "false") == 0)
			return "0";
		return value;
	}

	if (type == "int" || type == "enum") {
		try {
			return std::to_string(std::stoi(value));
		} catch (...) {
			return value;
		}
	}

	return value;
}

bool IsConfigValueDefault(const ConfigEntry &entry, const ConfigHint &hint) {
	const std::string currentValue = NormalizeConfigValue(entry.value, hint.type);
	const std::string defaultValue = NormalizeConfigValue(hint.defaultValue, hint.type);
	return currentValue == defaultValue;
}

void LoadAllConfigs() {
	const int count = 1 + (int)mods.size();
	g_allConfigs.resize(count);
	g_allHintMaps.resize(count);

	for (int i = 0; i < count; ++i) {
		const std::wstring configTypes = (i == 0) ? LOADER_CONFIG_TYPES_L : mods[i - 1].info.configTypes;
		const std::wstring configPath = GetConfigPath(i);
		const ParsedConfigHints parsedHints = ParseConfigHints(configTypes);

		g_allHintMaps[i] = parsedHints.lookup;
		g_allConfigs[i] = LauncherIni::ParseIniFile(configPath);
		const bool defaultsAdded = EnsureConfigDefaults(g_allConfigs[i], parsedHints.ordered);
		if (defaultsAdded) {
			LauncherIni::WriteIniFile(configPath, g_allConfigs[i]);
		}
	}
}

std::vector<ConfigSectionView> BuildConfigSections(const std::vector<ConfigEntry> &entries) {
	std::vector<ConfigSectionView> sections;
	std::map<std::string, size_t> sectionIndices;

	for (size_t i = 0; i < entries.size(); ++i) {
		const std::string sectionName = entries[i].section.empty() ? "Uncategorized" : entries[i].section;
		auto it = sectionIndices.find(sectionName);
		if (it == sectionIndices.end()) {
			sectionIndices[sectionName] = sections.size();
			sections.push_back({sectionName, {i}});
		} else {
			sections[it->second].entryIndices.push_back(i);
		}
	}

	return sections;
}

void RenderConfigSections() {
	auto &config = g_allConfigs[g_selectedMod];
	auto &hintMap = g_allHintMaps[g_selectedMod];
	const std::vector<ConfigSectionView> sections = BuildConfigSections(config);

	for (size_t sectionIndex = 0; sectionIndex < sections.size(); ++sectionIndex) {
		const ConfigSectionView &section = sections[sectionIndex];
		std::string headerLabel = section.name + "##section" + std::to_string(sectionIndex);

		if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::BeginTable(("##sectionTable" + std::to_string(sectionIndex)).c_str(), 3)) {
				ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 180.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, 72.0f);

				for (size_t entryIndex : section.entryIndices) {
					ConfigEntry &entry = config[entryIndex];

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted(entry.key.c_str());

					ImGui::TableSetColumnIndex(1);
					char buffer[256];
					strncpy_s(buffer, entry.value.c_str(), sizeof(buffer) - 1);

					ImGui::PushID(static_cast<int>(entryIndex));
					const std::string hintKey = entry.section + "/" + entry.key;
					const auto hintIt = hintMap.find(hintKey);
					const std::string type = (hintIt != hintMap.end()) ? hintIt->second.type : "string";
					bool valueChanged = false;

					ImGui::SetNextItemWidth(-FLT_MIN);

					if (type == "bool") {
						bool val = (entry.value == "1" || _stricmp(entry.value.c_str(), "true") == 0);
						if (ImGui::Checkbox("##value", &val)) {
							entry.value = val ? "1" : "0";
							valueChanged = true;
						}
					} else if (type == "int") {
						int val = 0;
						if (!entry.value.empty()) {
							try {
								val = std::stoi(entry.value);
							} catch (...) {
								val = 0;
							}
						}
						if (ImGui::InputInt("##value", &val)) {
							entry.value = std::to_string(val);
							valueChanged = true;
						}
					} else if (type == "enum") {
						int currentIndex = 0;
						if (!entry.value.empty()) {
							try {
								currentIndex = std::stoi(entry.value);
							} catch (...) {
								currentIndex = 0;
							}
						}
						if (currentIndex < 0 || currentIndex >= hintIt->second.options.size()) {
							currentIndex = 0;
						}
						std::vector<const char *> optionsCStr;
						for (const auto &option : hintIt->second.options) {
							optionsCStr.push_back(option.c_str());
						}
						if (ImGui::Combo("##value", &currentIndex, optionsCStr.data(), static_cast<int>(optionsCStr.size()))) {
							entry.value = std::to_string(currentIndex);
							valueChanged = true;
						}
					} else {
						if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
							entry.value = buffer;
							valueChanged = true;
						}
					}

					if (valueChanged) {
						SaveConfigForMod(g_selectedMod);
					}

					ImGui::TableSetColumnIndex(2);
					if (hintIt != hintMap.end()) {
						ImGui::BeginDisabled(IsConfigValueDefault(entry, hintIt->second));
						if (ImGui::Button("Reset##reset")) {
							entry.value = hintIt->second.defaultValue;
							SaveConfigForMod(g_selectedMod);
						}
						ImGui::EndDisabled();
					} else {
						ImGui::TextUnformatted("");
					}
					ImGui::PopID();
				}

				ImGui::EndTable();
			}

			if (sectionIndex + 1 < sections.size()) {
				ImGui::Spacing();
			}
		}
	}
}

void RenderSelectedItemDetails() {

	std::string name, version, author, description, hyperlink, filePath;
	int apiVersion = -1;
	if (g_selectedMod == 0) {
		name = LOADER_NAME;
		version = LOADER_VERSION;
		author = AUTHOR_NAME;
		description = LOADER_DESCRIPTION;
		hyperlink = LOADER_HYPERLINK;
		filePath = "";
		apiVersion = API_VERSION;
	} else {
		Mod &mod = mods[g_selectedMod - 1];
		name = WStringToString(mod.getName());
		version = WStringToString(mod.info.version);
		author = WStringToString(mod.info.author);
		description = WStringToString(mod.info.description);
		hyperlink = WStringToString(mod.info.hyperlink);
		filePath = WStringToString(mod.path.path);
		apiVersion = mod.info.apiVersion;
	}

	ImGui::Text(name.c_str());
	if (ImGui::IsItemHovered() && !filePath.empty()) {
		ImGui::SetTooltip("%s", filePath.c_str());
	}
	ImGui::Separator();
	ImGui::Spacing();

	if (!author.empty()) {
		ImGui::Text("Author: %s", author.c_str());
	}
	if (!version.empty() || apiVersion != -1) {
		ImGui::Text("Version: %s, API: v%d", version.c_str(), apiVersion);
	}
	if (!hyperlink.empty()) {
		ImGui::TextColored(ImVec4(0.25f, 0.5f, 1.0f, 1.0f), "Website URL");
		if (ImGui::IsItemClicked()) {
			ShellExecuteA(NULL, "open", hyperlink.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			ImGui::SetTooltip("%s", hyperlink.c_str());
		}
	}

	float descriptionHeight = ImGui::GetContentRegionAvail().y;
	if (descriptionHeight < 100.0f) {
		descriptionHeight = 100.0f;
	}

	ImGui::BeginChild("OverviewDescription", ImVec2(0.0f, descriptionHeight), true);
	if (!description.empty()) {
		ImGui::TextWrapped("%s", description.c_str());
	} else {
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No description provided.");
	}
	ImGui::EndChild();
}

void RenderSelectedItemConfig() {
	if (g_allConfigs[g_selectedMod].empty()) {
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No configuration available.");
	} else {
		RenderConfigSections();
	}
}

void RenderLicensesSection(bool fullHeight = false) {
	if (licenses.empty())
		return;

	float panelHeight = fullHeight ? ImGui::GetContentRegionAvail().y : 260.0f;
	if (panelHeight < 120.0f) {
		panelHeight = 120.0f;
	}

	ImGui::BeginChild("LicensePanel", ImVec2(0.0f, panelHeight), false);
	if (ImGui::BeginTabBar("LicenseTabs")) {
		for (size_t i = 0; i < licenses.size(); ++i) {
			if (ImGui::BeginTabItem(licenses[i].displayName)) {
				ImGui::BeginChild("LicenseContent", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
				ImGui::TextUnformatted(licenses[i].content);
				ImGui::EndChild();
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
	ImGui::EndChild();
}

} // namespace

namespace Launcher {

bool ShowLauncherWindow(HINSTANCE hInstance) {
	bool start_game = false;
	bool close_launcher = false;
	bool showLicensesOverlay = false;

	api->LogDebug("[Launcher] Setting up launcher window");

	if (!LauncherBackend::Initialize(hInstance, LOADER_NAME_L, minWindowWidth, minWindowHeight)) {
		api->LogError("[Launcher] Failed to initialize launcher window");
		return false;
	}

	api->LogDebug("[Launcher] Launcher window successfully created, initializing ImGui");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(LauncherBackend::GetWindowHandle());
	ImGui_ImplDX11_Init(LauncherBackend::GetDevice(), LauncherBackend::GetContext());

	RECT rect;
	GetClientRect(LauncherBackend::GetWindowHandle(), &rect);
	io.DisplaySize = ImVec2(
		static_cast<float>(rect.right - rect.left),
		static_cast<float>(rect.bottom - rect.top));

	api->LogDebug("[Launcher] ImGui initialized, entering main loop");

	LoadAllConfigs();

	while (!close_launcher) {
		LauncherBackend::PollMessages(close_launcher);
		if (close_launcher)
			break;

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::Begin(
			LOADER_NAME,
			nullptr,
			ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoTitleBar);

		float listWidth = 250.0f;
		float footerHeight = 16.0f;
		float contentHeight = ImGui::GetContentRegionAvail().y;
		if (!showLicensesOverlay) {
			contentHeight -= (footerHeight + ImGui::GetStyle().ItemSpacing.y);
			if (contentHeight < 0.0f) {
				contentHeight = 0.0f;
			}
		}

		if (showLicensesOverlay) {
			ImGui::BeginChild("LicensesOverlayPane", ImVec2(0, contentHeight), true);
			if (ImGui::Button("Back")) {
				showLicensesOverlay = false;
			}
			ImGui::SameLine();
			ImGui::TextUnformatted("Licenses");
			ImGui::Separator();
			ImGui::Spacing();
			RenderLicensesSection(true);
			ImGui::EndChild();
		} else {
			ImGui::BeginChild("ListPane", ImVec2(listWidth, contentHeight), true);
			{
				ImGui::TextUnformatted("Mod List");

				ImGui::Spacing();

				ImGui::BeginChild("ModList", ImVec2(listWidth - 16.0f, contentHeight - 80.0f), true);
				{
					float availableWidth = ImGui::GetContentRegionAvail().x;
					float nameWidth = availableWidth - 20.0f;
					float checkboxOffset = availableWidth - 5.0f;

					// Loader entry
					bool loaderSelected = (g_selectedMod == 0);
					ImGui::AlignTextToFramePadding();
					if (ImGui::Selectable((std::string(LOADER_NAME) + "##loader").c_str(), loaderSelected, 0, ImVec2(nameWidth, 0))) {
						g_selectedMod = 0;
					}
					ImGui::SameLine(checkboxOffset);
					if (ImGui::Checkbox("##loaderCheckbox", &loaderEnabled)) {
						api->WriteIniBool(L"Config", L"LoaderEnabled", loaderEnabled);
						LoadAllConfigs();
					}

					// User mods
					for (size_t i = 0; i < mods.size(); ++i) {
						bool isSelected = (g_selectedMod == (int)(i + 1));
						std::string modName = WStringToString(mods[i].getName());
						std::string selectableName = modName + "##mod" + std::to_string(i);

						ImGui::BeginDisabled(!loaderEnabled);
						ImGui::AlignTextToFramePadding();
						if (ImGui::Selectable(selectableName.c_str(), isSelected, 0, ImVec2(nameWidth, 0))) {
							g_selectedMod = (int)(i + 1);
						}
						ImGui::SameLine(checkboxOffset);
						if (ImGui::Checkbox(("##checkbox" + std::to_string(i)).c_str(), &mods[i].enabled)) {
							SaveDisabledMods(mods);
							LoadAllConfigs();
						}
						ImGui::EndDisabled();
					}
				}
				ImGui::EndChild();

				ImGui::Spacing();

				if (ImGui::Button("Launch Game", ImVec2(listWidth - 16.0f, 32.0f))) {
					start_game = true;
					close_launcher = true;
				}
			}
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("DetailsPane", ImVec2(0, contentHeight), true);
			{
				const bool hasConfig = !g_allConfigs[g_selectedMod].empty();
				if (ImGui::BeginTabBar("DetailsTabs")) {
					if (ImGui::BeginTabItem("Overview")) {
						RenderSelectedItemDetails();
						ImGui::EndTabItem();
					}

					if (!hasConfig)
						ImGui::BeginDisabled();
					if (ImGui::BeginTabItem("Settings")) {
						RenderSelectedItemConfig();
						ImGui::EndTabItem();
					}
					if (!hasConfig)
						ImGui::EndDisabled();

					ImGui::EndTabBar();
				}
			}
			ImGui::EndChild();

			ImGui::BeginChild("FooterBar", ImVec2(0, footerHeight), false);
			{
				float footerTextY = (footerHeight - ImGui::GetTextLineHeight()) * 0.5f;
				if (footerTextY < 0.0f) {
					footerTextY = 0.0f;
				}

				const std::string footerLeft = std::string(LOADER_NAME) + " v" + std::string(LOADER_VERSION);
				ImGui::SetCursorPosY(footerTextY);
				ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.75f, 1.0f), "%s", footerLeft.c_str());

				const char *legalLink = "Licenses";
				float linkWidth = ImGui::CalcTextSize(legalLink).x;
				float rightX = ImGui::GetWindowContentRegionMax().x - linkWidth;
				if (rightX > ImGui::GetCursorPosX()) {
					ImGui::SameLine(rightX);
				} else {
					ImGui::SameLine();
				}

				ImGui::SetCursorPosY(footerTextY);
				ImGui::TextColored(ImVec4(0.25f, 0.5f, 1.0f, 1.0f), "%s", legalLink);
				if (ImGui::IsItemClicked()) {
					showLicensesOverlay = true;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				}
			}
			ImGui::EndChild();
		}

		ImGui::End();

		ImGui::Render();
		const float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.00f};
		LauncherBackend::BeginFrame(clearColor);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		LauncherBackend::Present(1, 0);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	LauncherBackend::Shutdown(hInstance);

	return start_game;
}

} // namespace Launcher
