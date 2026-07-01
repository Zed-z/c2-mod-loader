#include "LevelSelect.h"

#include "ModApi.h"
#include "Utils/Fonts.h"
#include "WadNames.h"
#include "imgui.h"

#include <cctype>
#include <filesystem>
#include <map>
#include <regex>
#include <sstream>
#include <vector>

extern ModApi *api;

namespace fs = std::filesystem;

namespace {

struct WadLevel {
	int tribe;
	int level;
	int map;
	WadFileType type;
	std::string filename;
};

using WadLevelMap = std::map<std::tuple<int, int, int, int>, WadLevel>;

std::map<int, std::vector<WadLevel>> discoveredLevels;
bool levelsDiscovered = false;

WadFileType charToWadType(char c) {
	switch (std::tolower(c)) {
	case 'l':
		return WadFileType::WAD_TYPE_LEVEL;
	case 'b':
		return WadFileType::WAD_TYPE_BOSS;
	case 's':
		return WadFileType::WAD_TYPE_SECRET;
	case 'i':
		return WadFileType::WAD_TYPE_CUTSCENE;
	default:
		return WadFileType::WAD_TYPE_LEVEL;
	}
}

void ScanWadsDirectory(const fs::path &dirPath, const std::regex &wadPattern, WadLevelMap &wadLevelMap) {
	for (const auto &entry : fs::directory_iterator(dirPath)) {
		if (!entry.is_regular_file())
			continue;

		std::string filename = entry.path().filename().string();
		std::smatch match;
		if (!std::regex_match(filename, match, wadPattern))
			continue;

		try {
			int tribe = std::stoi(match[1].str());
			char typeChar = std::tolower(match[2].str()[0]);
			int level = std::stoi(match[3].str());
			int map = std::stoi(match[4].str());
			WadFileType type = charToWadType(typeChar);

			auto key = std::make_tuple(tribe, static_cast<int>(type), level, map);
			wadLevelMap[key] = {tribe, level, map, type, filename};
		} catch (...) {
			continue;
		}
	}
}

void DiscoverWadFiles() {
	discoveredLevels.clear();

	try {
		wchar_t exePath[MAX_PATH];
		GetModuleFileNameW(nullptr, exePath, MAX_PATH);
		fs::path gameDir = fs::path(exePath).parent_path();
		fs::path wadsDir = gameDir / L"Wads";
		fs::path overridesWadsDir = gameDir / L"mods" / L".overrides" / L"Wads";

		if (!fs::exists(wadsDir)) {
			api->LogWarning("Wads directory not found");
			levelsDiscovered = true;
			return;
		}

		WadLevelMap wadLevelMap;

		std::regex wadPattern(R"(t(\d+)([lbsi])(\d+)m(\d+)\.wad)", std::regex::icase);

		ScanWadsDirectory(wadsDir, wadPattern, wadLevelMap);
		if (fs::exists(overridesWadsDir)) {
			ScanWadsDirectory(overridesWadsDir, wadPattern, wadLevelMap);
		}

		for (const auto &[key, wadLevel] : wadLevelMap) {
			discoveredLevels[wadLevel.tribe].push_back(wadLevel);
		}

		// Sort levels per tribe
		for (auto &[tribe, levels] : discoveredLevels) {
			std::sort(levels.begin(), levels.end(),
				[](const WadLevel &a, const WadLevel &b) {
					if (a.type != b.type)
						return static_cast<int>(a.type) < static_cast<int>(b.type);
					if (a.level != b.level)
						return a.level < b.level;
					return a.map < b.map;
				});
		}

		levelsDiscovered = true;
	} catch (const std::exception &e) {
		api->LogError(std::string("Error discovering WAD files: ").append(e.what()).c_str());
		levelsDiscovered = true;
	}
}

} // namespace

namespace Overlay::LevelSelect {

bool showLevelSelect;

void Setup() {
	showLevelSelect = api->SetupIniBool(L"GUI", L"ShowLevelSelect", false);
}

void RenderLevelSelect() {
	if (!showLevelSelect)
		return;

	bool prevShow = showLevelSelect;

	if (!levelsDiscovered) {
		DiscoverWadFiles();
	}

	ImGui::Begin("Level Select", &showLevelSelect);

	if (discoveredLevels.empty()) {
		ImGui::Text("No WAD files found in Wads/ directory");
	} else {
		for (const auto &[tribe, levels] : discoveredLevels) {
			ImGui::PushID(tribe);
			std::string tribeName = "Tribe " + std::to_string(tribe);

			if (ImGui::CollapsingHeader(tribeName.c_str())) {

				// Group by type + level
				std::map<std::pair<int, int>, std::vector<const WadLevel *>> levelMap;
				for (const auto &wadLevel : levels) {
					levelMap[{static_cast<int>(wadLevel.type), wadLevel.level}].push_back(&wadLevel);
				}

				for (const auto &[typeAndLevel, levelMaps] : levelMap) {
					WadFileType type = static_cast<WadFileType>(typeAndLevel.first);
					int levelNum = typeAndLevel.second;
					std::string levelHeader = std::string(wadFileTypeNames[type + 1]) + " " + std::to_string(levelNum);

					ImGui::Separator();
					ImGui::Text("%s", levelHeader.c_str());
					ImGui::Spacing();

					for (const auto *wadLevel : levelMaps) {
						std::string lookupKey = "wads/" + wadLevel->filename;
						std::string wadName = wadNames.count(lookupKey) ? wadNames[lookupKey] : "";
						if (!wadName.empty()) {
							ImGui::Text("%s", wadName.c_str());
						} else {
							ImGui::Text("%s", wadLevel->filename.c_str());
						}
						std::string buttonLabel = "Map " + std::to_string(wadLevel->map) + " (" + wadLevel->filename + ")";
						if (ImGui::Button(buttonLabel.c_str())) {
							api->GotoLevel(wadLevel->tribe, wadLevel->level, wadLevel->map, wadLevel->type);
						}
						ImGui::Spacing();
					}

					ImGui::Spacing();
				}
			}
			ImGui::PopID();
		}
	}
	ImGui::End();

	if (prevShow != showLevelSelect) {
		api->WriteIniBool(L"GUI", L"ShowLevelSelect", showLevelSelect);
	}
}

} // namespace Overlay::LevelSelect
