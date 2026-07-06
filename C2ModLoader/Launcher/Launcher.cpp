#include "Launcher.h"
#include "Backend.h"
#include "Config.h"
#include "EmbeddedLicenses.h"
#include "IniConfig.h"
#include "Loader.h"
#include "ModApi.h"
#include "Resource.h"
#include "Utils.h"
#include "Utils/Fonts.h"
#include "Utils/ResourceLoader.h"
#include "Utils/Style.h"

#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
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
float g_uiScale = 1.0f;

constexpr float fontSizeTitle = 22.0f;
constexpr float fontSizeText = 16.0f;
static ID3D11ShaderResourceView *g_launcherLogoTexture = nullptr;
static float g_launcherLogoWidth = 0.0f;
static float g_launcherLogoHeight = 0.0f;

extern bool loaderEnabled;

namespace {
using ConfigEntry = LauncherIni::ConfigEntry;

using ConfigSectionKey = std::string;
using ConfigKey = std::string;

struct ConfigSection {
	std::string name;
	std::string description;
};

struct ConfigSectionView {
	ConfigSection sectionInfo;
	std::vector<size_t> entryIndices;
};

struct ConfigHint {
	bool hidden = false;
	std::string type;
	std::string defaultValue;
	std::string name;
	std::string description;
	std::vector<std::string> options; // Enum options for dropdowns
};

struct ParsedConfig {
	struct {
		std::vector<std::pair<ConfigSectionKey, ConfigSection>> ordered;
		std::map<ConfigSectionKey, ConfigSection> lookup;
	} sections;
	struct {
		std::vector<std::pair<ConfigKey, ConfigHint>> ordered;
		std::map<ConfigKey, ConfigHint> lookup;
	} hints;
	bool hasTypeHints = false;
	std::vector<ConfigEntry> configEntries;
};

// 0 = loader, 1+ = mods
int g_selectedMod = 0;
std::vector<ParsedConfig> allParsedConfigs;

std::vector<Launcher::EmbeddedLicense> licenses = Launcher::GetEmbeddedLicenses();

std::wstring GetConfigPath(int modIndex) {
	if (modIndex == 0) { // TODO: possibly overkill
		wchar_t exePath[MAX_PATH] = {};
		GetModuleFileNameW(GetModuleHandleA(NULL), exePath, MAX_PATH);
		PathInfo exeInfo = GetPathInfo(std::wstring(exePath));
		return exeInfo.directory + CONFIG_FILE_L;
	}
	std::wstring path = mods[modIndex - 1].path.path;
	path.replace(path.length() - 4, 4, L".ini");
	return path;
}

void SaveConfigForMod(int modIndex) {
	LauncherIni::WriteIniFile(GetConfigPath(modIndex), allParsedConfigs[modIndex].configEntries);
}

ParsedConfig ParseConfigHints(const std::wstring &configTypes) {
	ParsedConfig parsedConfig;
	if (configTypes.empty())
		return parsedConfig;

	parsedConfig.hasTypeHints = true;

	std::string str = WStringToString(configTypes);
	std::istringstream ss(str);
	std::string token;

	while (std::getline(ss, token, ';')) {
		/*
			Config Section: @Section[Name|Description]
				@ - Section marker
				Name - Human readable name
				Description - Description for display below the header
			Config Hint:    [_]Section/Key[Name|Description]:type[typeinfo]=default
				_ - Mark as hidden
				Name - Human readable name
				Description - Description for tooltips
				typeinfo - Additional info for certain types:
					enum[Option1|Option2|Option3]
		*/
		size_t atPos = token.find('@');
		bool isSection = atPos != std::string::npos && atPos == 0;

		if (isSection) {
			std::string sectionKey = token.substr(1);
			size_t openBracketPos = token.find('[');
			size_t closeBracketPos = token.find(']');
			if (openBracketPos != std::string::npos && openBracketPos > 1) {
				sectionKey = token.substr(1, openBracketPos - 1);
			}

			std::string sectionName = (openBracketPos != std::string::npos && closeBracketPos != std::string::npos && closeBracketPos > openBracketPos)
				? token.substr(openBracketPos + 1, closeBracketPos - openBracketPos - 1)
				: sectionKey;
			std::string name, description;
			size_t pipePos = sectionName.find('|');
			if (pipePos != std::string::npos) {
				name = sectionName.substr(0, pipePos);
				description = sectionName.substr(pipePos + 1);
			} else {
				name = sectionName;
				description = "";
			}
			parsedConfig.sections.ordered.push_back({sectionKey, {name, description}});
			parsedConfig.sections.lookup[sectionKey] = {name, description};
		} else {
			bool hidden = false;
			size_t hiddenMarkerPos = token.find('_');
			if (hiddenMarkerPos != std::string::npos) {
				token.erase(hiddenMarkerPos, 1);
				hidden = true;
			}
			size_t colonPos = token.find(':');
			if (colonPos == std::string::npos)
				continue;
			size_t equalsPos = token.find('=', colonPos + 1);
			if (equalsPos == std::string::npos)
				continue;

			std::string name, description;
			std::string keyString = token.substr(0, colonPos);
			size_t openBracketPos = keyString.find('[');
			size_t closeBracketPos = keyString.find(']');
			if (!(openBracketPos == std::string::npos || closeBracketPos == std::string::npos || closeBracketPos < openBracketPos)) {
				const std::string nameDescStr = keyString.substr(openBracketPos + 1, closeBracketPos - openBracketPos - 1);
				size_t pipePos = nameDescStr.find('|');
				name = (pipePos == std::string::npos) ? nameDescStr : nameDescStr.substr(0, pipePos);
				description = (pipePos == std::string::npos) ? "" : nameDescStr.substr(pipePos + 1);
				keyString = keyString.substr(0, openBracketPos);
			} else {
				size_t slashPos = keyString.find('/');
				name = (slashPos == std::string::npos) ? keyString : keyString.substr(slashPos + 1);
				description = "";
			}

			ConfigHint hint;
			hint.hidden = hidden;
			hint.name = name;
			hint.description = description;
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

			parsedConfig.hints.ordered.push_back({keyString, hint});
			parsedConfig.hints.lookup[keyString] = hint;
		}
	}
	return parsedConfig;
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

bool IsConfigEntryDeclared(const ParsedConfig &parsedConfig, const ConfigEntry &entry) {
	if (!parsedConfig.hasTypeHints)
		return true;

	const std::string hintKey = entry.section + "/" + entry.key;
	return parsedConfig.hints.lookup.find(hintKey) != parsedConfig.hints.lookup.end();
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

	if (type == "float") {
		try {
			const float parsed = std::stof(value);
			char buffer[64] = {};
			std::snprintf(buffer, sizeof(buffer), "%.8g", parsed);
			return std::string(buffer);
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

void SortConfigEntriesByHints(std::vector<ConfigEntry> &entries, const std::vector<std::pair<std::string, ConfigHint>> &orderedHints) {
	// Hint order map
	std::map<std::string, size_t> hintOrderMap;
	for (size_t i = 0; i < orderedHints.size(); ++i) {
		std::string keyPath = orderedHints[i].first;
		std::transform(keyPath.begin(), keyPath.end(), keyPath.begin(), [](unsigned char c) { return std::tolower(c); });
		hintOrderMap[keyPath] = i;
	}

	// Stable sort
	std::stable_sort(entries.begin(), entries.end(), [&hintOrderMap](const ConfigEntry &a, const ConfigEntry &b) {
		std::string keyA = a.section + "/" + a.key;
		std::string keyB = b.section + "/" + b.key;
		std::transform(keyA.begin(), keyA.end(), keyA.begin(), [](unsigned char c) { return std::tolower(c); });
		std::transform(keyB.begin(), keyB.end(), keyB.begin(), [](unsigned char c) { return std::tolower(c); });

		auto itA = hintOrderMap.find(keyA);
		auto itB = hintOrderMap.find(keyB);

		// No hint - push to end
		size_t indexA = (itA != hintOrderMap.end()) ? itA->second : static_cast<size_t>(-1);
		size_t indexB = (itB != hintOrderMap.end()) ? itB->second : static_cast<size_t>(-1);

		return indexA < indexB;
	});
}

void LoadAllConfigs() {
	const int count = 1 + (int)mods.size();
	allParsedConfigs.resize(count);

	for (int i = 0; i < count; ++i) {
		const std::wstring configTypes = (i == 0 ? modLoader.info.configTypes : mods[i - 1].info.configTypes);
		const std::wstring configPath = GetConfigPath(i);
		ParsedConfig parsedConfig = ParseConfigHints(configTypes);
		parsedConfig.configEntries = LauncherIni::ParseIniFile(configPath);

		const bool defaultsAdded = EnsureConfigDefaults(parsedConfig.configEntries, parsedConfig.hints.ordered);

		SortConfigEntriesByHints(parsedConfig.configEntries, parsedConfig.hints.ordered);

		if (defaultsAdded) {
			LauncherIni::WriteIniFile(configPath, parsedConfig.configEntries);
		}

		allParsedConfigs[i] = parsedConfig;
	}
}

std::vector<ConfigSectionView> BuildConfigSections(const ParsedConfig &parsedConfig) {
	std::vector<ConfigSectionView> sections;
	std::map<ConfigSectionKey, size_t> sectionIndices;
	const bool showFallbackEntries = !parsedConfig.hasTypeHints;

	for (const auto &[sectionKey, section] : parsedConfig.sections.ordered) {
		sectionIndices[sectionKey] = sections.size();
		sections.push_back({section, {}});
	}

	const auto &entries = parsedConfig.configEntries;
	for (size_t i = 0; i < entries.size(); ++i) {
		if (!showFallbackEntries && !IsConfigEntryDeclared(parsedConfig, entries[i])) {
			continue;
		}

		const std::string hintKey = entries[i].section + "/" + entries[i].key;
		const auto hintIt = parsedConfig.hints.lookup.find(hintKey);
		if (hintIt != parsedConfig.hints.lookup.end() && hintIt->second.hidden) {
			continue;
		}

		ConfigSectionKey key = entries[i].section.empty() ? "Uncategorized" : entries[i].section;
		ConfigSection sectionInfo;
		const auto sectionIt = parsedConfig.sections.lookup.find(key);
		if (sectionIt != parsedConfig.sections.lookup.end()) {
			sectionInfo = {sectionIt->second.name, sectionIt->second.description};
		} else {
			sectionInfo = {key, ""};
		}

		auto it = sectionIndices.find(key);
		if (it == sectionIndices.end()) {
			sectionIndices[key] = sections.size();
			sections.push_back({sectionInfo, {i}});
		} else {
			sections[it->second].entryIndices.push_back(i);
		}
	}

	// Erase empty sections
	sections.erase(
		std::remove_if(sections.begin(), sections.end(), [](const ConfigSectionView &section) {
			return section.entryIndices.empty();
		}),
		sections.end());

	return sections;
}

void RenderConfigSections(const std::vector<ConfigSectionView> &sections) {
	ParsedConfig &parsedConfig = allParsedConfigs[g_selectedMod];
	for (size_t sectionIndex = 0; sectionIndex < sections.size(); ++sectionIndex) {
		const ConfigSectionView &section = sections[sectionIndex];
		std::string headerLabel = section.sectionInfo.name + "##section" + std::to_string(sectionIndex);
		const bool isOpen = ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

		if (isOpen) {
			if (!section.sectionInfo.description.empty()) {
				ImGui::Spacing();
				ImGui::TextWrapped(section.sectionInfo.description.c_str());
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
			}

			if (ImGui::BeginTable(("##sectionTable" + std::to_string(sectionIndex)).c_str(), 3, ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch, 2.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.0f);
				ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, 48.0f * g_uiScale);

				for (size_t entryIndex : section.entryIndices) {
					ConfigEntry &entry = parsedConfig.configEntries[entryIndex];
					const std::string hintKey = entry.section + "/" + entry.key;
					const auto hintIt = parsedConfig.hints.lookup.find(hintKey);
					const std::string type = (hintIt != parsedConfig.hints.lookup.end()) ? hintIt->second.type : "string";
					const std::string name = (hintIt != parsedConfig.hints.lookup.end()) ? hintIt->second.name : entry.key;
					const std::string description = (hintIt != parsedConfig.hints.lookup.end()) ? hintIt->second.description : "";
					bool valueChanged = false;

					if (hintIt != parsedConfig.hints.lookup.end() && hintIt->second.hidden) {
						continue;
					}

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);

					ImGui::AlignTextToFramePadding();
					ImGui::TextUnformatted(name.c_str());

					if (!description.empty()) {
						ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
						ImGui::Indent();
						ImGui::TextWrapped("%s", description.c_str());
						ImGui::Unindent();
						ImGui::PopStyleColor();
					}

					ImGui::Spacing();
					ImGui::Spacing();

					ImGui::TableSetColumnIndex(1);
					char buffer[256];
					strncpy_s(buffer, entry.value.c_str(), sizeof(buffer) - 1);

					ImGui::PushID(static_cast<int>(entryIndex));

					ImGui::SetNextItemWidth(-FLT_MIN);

					if (!description.empty()) {
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Spacing();
					}

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
					} else if (type == "float") {
						float val = 0.0f;
						if (!entry.value.empty()) {
							try {
								val = std::stof(entry.value);
							} catch (...) {
								val = 0.0f;
							}
						}
						if (ImGui::InputFloat("##value", &val, 0.0f, 0.0f, "%.6g")) {
							char floatBuffer[64] = {};
							std::snprintf(floatBuffer, sizeof(floatBuffer), "%.8g", val);
							entry.value = floatBuffer;
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
					if (hintIt != parsedConfig.hints.lookup.end()) {

						if (!description.empty()) {
							ImGui::Spacing();
							ImGui::Spacing();
							ImGui::Spacing();
							ImGui::Spacing();
						}

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

	Mod &mod = g_selectedMod == 0 ? modLoader : mods[g_selectedMod - 1];
	std::string name = WStringToString(mod.getName());
	std::string version = WStringToString(mod.info.version);
	std::string author = WStringToString(mod.info.author);
	std::string description = WStringToString(mod.info.description);
	std::string hyperlink = WStringToString(mod.info.hyperlink);
	std::string filePath = WStringToString(mod.path.path);
	std::string fileHash = mod.fileHash;
	int apiVersion = mod.info.apiVersion;

	ImGui::PushFont(Fonts::GetFontTitle(), fontSizeTitle);
	ImGui::Text(name.c_str());
	ImGui::PopFont();

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
		ImGui::TextColored(Style::GetStyle().link, "Website URL");
		if (ImGui::IsItemClicked()) {
			ShellExecuteA(NULL, "open", hyperlink.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			ImGui::SetTooltip("%s", hyperlink.c_str());
		}
	}
	ImGui::PushFont(Fonts::GetFontCode(), 13);
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
	ImGui::TextWrapped("SHA256: %s", fileHash.empty() ? "N/A" : fileHash.c_str());
	ImGui::PopStyleColor();
	ImGui::PopFont();

	ImGui::BeginChild("OverviewDescription", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NavFlattened);
	if (!description.empty()) {
		ImGui::TextWrapped("%s", description.c_str());
	} else {
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No description provided.");
	}
	ImGui::EndChild();
}

void RenderSelectedItemConfig() {
	ParsedConfig &parsedConfig = allParsedConfigs[g_selectedMod];
	const std::vector<ConfigSectionView> sections = BuildConfigSections(parsedConfig);
	if (sections.empty()) {
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No configuration available.");
	} else {
		if (ImGui::BeginChild("SettingsScrollRegion", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NavFlattened)) {
			RenderConfigSections(sections);
		}
		ImGui::EndChild();
	}
}

void RenderSelectedItemFileOverrides() {
	const Mod &mod = g_selectedMod == 0 ? modLoader : mods[g_selectedMod - 1];
	if (mod.fileOverrides.empty()) {
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No file overrides available.");
	} else {
		const std::wstring &overridePath = mod.overridePath;
		const bool overridePathExists = !mod.fileOverrides.empty();

		ImGui::BeginDisabled(!overridePathExists);
		if (ImGui::Button("Open Folder")) {
			ShellExecuteW(NULL, L"open", overridePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered() && overridePathExists) {
			ImGui::SetTooltip("%s", WStringToString(overridePath).c_str());
		}

		ImGui::Spacing();

		if (ImGui::BeginChild("OverridesScrollRegion", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NavFlattened)) {
			if (ImGui::BeginTable("##fileoverridestable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
				ImGui::TableHeadersRow();

				for (const auto &override : mod.fileOverrides) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					std::string displayPath = WStringToString(override.relativePath) + (override.isDirectory ? "\\" : "");
					ImGui::TextUnformatted(displayPath.c_str());

					ImGui::TableSetColumnIndex(1);
					if (!override.isDirectory) {
						if (override.fileSize < 1024) {
							ImGui::Text("%u B", override.fileSize);
						} else if (override.fileSize < 1024 * 1024) {
							ImGui::Text("%.2f KB", override.fileSize / 1024.0f);
						} else {
							ImGui::Text("%.2f MB", override.fileSize / (1024.0f * 1024.0f));
						}
					} else {
						ImGui::TextUnformatted("[DIR]");
					}
				}
				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
	}
}

void RenderLicensesSection(bool fullHeight = false) {
	if (licenses.empty())
		return;

	float panelHeight = fullHeight ? ImGui::GetContentRegionAvail().y : (260.0f * g_uiScale);
	if (panelHeight < (120.0f * g_uiScale)) {
		panelHeight = 120.0f * g_uiScale;
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

bool LoadLauncherLogoTexture() {
	if (g_launcherLogoTexture) {
		return true;
	}

	ID3D11Device *device = LauncherBackend::GetDevice();
	if (!device) {
		return false;
	}

	HMODULE module = GetCallingModule();
	if (!module) {
		return false;
	}

	if (!ResourceLoader::LoadTextureFromPngResource(module, IDR_LOGO, device, &g_launcherLogoTexture, &g_launcherLogoWidth, &g_launcherLogoHeight)) {
		g_launcherLogoWidth = 0.0f;
		g_launcherLogoHeight = 0.0f;
		return false;
	}

	return true;
}

void CleanupLauncherLogoTexture() {
	if (g_launcherLogoTexture) {
		g_launcherLogoTexture->Release();
		g_launcherLogoTexture = nullptr;
	}
	g_launcherLogoWidth = 0.0f;
	g_launcherLogoHeight = 0.0f;
}

void UpdateUiScale(ImGuiIO &io, const ImGuiStyle &baseStyle, float scale) {
	ImGuiStyle &style = ImGui::GetStyle();
	style = baseStyle;
	style.ScaleAllSizes(scale);
	io.FontGlobalScale = scale;
	g_uiScale = scale;
}

} // namespace

namespace Launcher {

bool ShowLauncherWindow(HINSTANCE hInstance) {
	bool start_game = false;
	bool close_launcher = false;
	bool showLicensesOverlay = false;
	const bool unknownGameVersion = (api->GetGameVersion() == GAMEVER_UNKNOWN);
	bool unknownVersionPopupPending = unknownGameVersion;

	api->LogDebug("[Launcher] Setting up launcher window");

	if (!LauncherBackend::Initialize(hInstance, LOADER_NAME_L, minWindowWidth, minWindowHeight)) {
		api->LogError("[Launcher] Failed to initialize launcher window");
		return false;
	}

	api->LogDebug("[Launcher] Launcher window successfully created, initializing ImGui");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();

	io.Fonts->Clear();
	io.FontDefault = Fonts::GetFontText();

	ImGuiStyle baseStyle;
	ImGui::StyleColorsDark();
	Style::ApplyStyle();
	baseStyle = ImGui::GetStyle();
	UpdateUiScale(io, baseStyle, LauncherBackend::GetDpiScale());

	ImGui_ImplWin32_Init(LauncherBackend::GetWindowHandle());
	ImGui_ImplDX11_Init(LauncherBackend::GetDevice(), LauncherBackend::GetContext());

	if (!LoadLauncherLogoTexture()) {
		api->LogWarning("[Launcher] Failed to load embedded launcher logo texture, using text header fallback");
	}

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
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_MenuBar);

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Open Game Folder")) {
					ShellExecuteW(NULL, L"open", modLoader.path.directory.c_str(), NULL, NULL, SW_SHOWNORMAL);
				}
				if (ImGui::MenuItem("Open Mods Folder")) {
					ShellExecuteW(NULL, L"open", MOD_DIRECTORY_L, NULL, NULL, SW_SHOWNORMAL);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("View Licenses")) {
					showLicensesOverlay = true;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Exit")) {
					start_game = false;
					close_launcher = true;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		if (unknownVersionPopupPending) {
			ImGui::OpenPopup("Unknown Game Version");
			unknownVersionPopupPending = false;
		}

		const float listWidth = 250.0f * g_uiScale;
		const float footerHeight = 16.0f * g_uiScale;
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
				if (g_launcherLogoTexture && g_launcherLogoWidth > 0.0f && g_launcherLogoHeight > 0.0f) {
					const float availableWidth = ImGui::GetContentRegionAvail().x;
					const float maxLogoHeight = 56.0f * g_uiScale;
					float logoScale = maxLogoHeight / g_launcherLogoHeight;
					const float widthScale = availableWidth / g_launcherLogoWidth;
					if (widthScale < logoScale) {
						logoScale = widthScale;
					}
					if (logoScale > 1.0f) {
						logoScale = 1.0f;
					}
					if (logoScale <= 0.0f) {
						logoScale = 1.0f;
					}

					const float renderWidth = g_launcherLogoWidth * logoScale;
					const float renderHeight = g_launcherLogoHeight * logoScale;
					const float cursorX = ImGui::GetCursorPosX();
					if (renderWidth < availableWidth) {
						ImGui::SetCursorPosX(cursorX + (availableWidth - renderWidth) * 0.5f);
					}
					ImGui::Image((ImTextureID)g_launcherLogoTexture, ImVec2(renderWidth, renderHeight));
				} else {
					ImGui::PushFont(Fonts::GetFontTitle(), fontSizeTitle);
					ImGui::TextUnformatted(LOADER_NAME);
					ImGui::PopFont();
				}

				ImGui::Spacing();

				ImGui::BeginChild("ModList", ImVec2(listWidth - (16.0f * g_uiScale), contentHeight - (88.0f * g_uiScale)), true);
				{
					float availableWidth = ImGui::GetContentRegionAvail().x;
					float nameWidth = availableWidth - (25.0f * g_uiScale);
					float checkboxOffset = availableWidth - (10.0f * g_uiScale);

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

				if (api->GetGameVersion() == GAMEVER_UNKNOWN) {
					ImGui::BeginDisabled();
					ImGui::Button("Unsupported Game", ImVec2(listWidth - (16.0f * g_uiScale), 32.0f * g_uiScale));
					ImGui::EndDisabled();
				} else {
					if (ImGui::Button("Launch Game", ImVec2(listWidth - (16.0f * g_uiScale), 32.0f * g_uiScale))) {
						start_game = true;
						close_launcher = true;
					}
				}
			}
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild("DetailsPane", ImVec2(0, contentHeight), true);
			{
				const bool hasConfig = !allParsedConfigs[g_selectedMod].configEntries.empty();
				const bool hasFileOverrides = !(g_selectedMod == 0 ? modLoader : mods[g_selectedMod - 1]).fileOverrides.empty();
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

					if (!hasFileOverrides)
						ImGui::BeginDisabled();
					if (ImGui::BeginTabItem("Files")) {
						RenderSelectedItemFileOverrides();
						ImGui::EndTabItem();
					}
					if (!hasFileOverrides)
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

				const std::string footerLeft = std::string(LOADER_NAME) + " v" + std::string(LOADER_VERSION) + "   |   " + "Game Version: " + GameVersions[api->GetGameVersion()];
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
				ImGui::TextColored(Style::GetStyle().link, "%s", legalLink);
				if (ImGui::IsItemClicked()) {
					showLicensesOverlay = true;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				}
			}
			ImGui::EndChild();
		}

		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(400.0f * g_uiScale, 0.0f), ImGuiCond_Appearing);
		if (ImGui::BeginPopupModal("Unknown Game Version", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::TextWrapped("Could not determine the game's version.");
			ImGui::Spacing();
			ImGui::TextWrapped("Make sure the game is unmodified and acquired from legitimate sources.");
			ImGui::Spacing();
			ImGui::TextWrapped("The launch button will be disabled.");
			ImGui::Spacing();
			if (ImGui::Button("OK", ImVec2(120.0f * g_uiScale, 0.0f))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::End();

		ImGui::Render();
		const float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.00f};
		LauncherBackend::BeginFrame(clearColor);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		LauncherBackend::Present(1, 0);
	}

	CleanupLauncherLogoTexture();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	LauncherBackend::Shutdown(hInstance);

	return start_game;
}

} // namespace Launcher
