#include "Launcher.h"
#include "Backend.h"
#include "IniConfig.h"
#include "ModApi.h"
#include "Config.h"
#include "Loader.h"
#include "Utils.h"
#include "Resource.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include <d3d11.h>
#include <shellapi.h>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#pragma comment(lib, "shell32.lib")

extern ModApi* api;

int minWindowWidth = 720;
int minWindowHeight = 480;

namespace {
    using ConfigEntry = LauncherIni::ConfigEntry;

    struct ConfigSectionView {
        std::string name;
        std::vector<size_t> entryIndices;
    };

    struct ConfigHint {
        std::string type;
        std::string defaultValue;
    };

    int g_selectedMod = 0; // 0 = C2ModLoader, 1+ = mod index  
    std::vector<std::vector<ConfigEntry>> g_allConfigs;       // index 0 = loader, 1+ = mods
    std::vector<std::map<std::string, ConfigHint>> g_allHintMaps;

    void SaveConfigForMod(int modIndex);

    std::wstring GetConfigPath(int modIndex) {
        if (modIndex == 0) return CONFIG_FILE_L;
        std::wstring path = mods[modIndex - 1].path.path;
        path.replace(path.length() - 4, 4, L".ini");
        return path;
    }

    std::map<std::string, ConfigHint> ParseConfigHints(const std::wstring& configTypes) {
        std::map<std::string, ConfigHint> hintMap;
        if (configTypes.empty()) return hintMap;

        std::string str = WStringToString(configTypes);
        std::istringstream ss(str);
        std::string token;

        while (std::getline(ss, token, ',')) {
            // Expected format: "Section/Key:type:default"
            size_t firstColon = token.find(':');
            if (firstColon == std::string::npos) continue;
            size_t secondColon = token.find(':', firstColon + 1);
            if (secondColon == std::string::npos) continue;

            const std::string keyPath = token.substr(0, firstColon);
            ConfigHint hint;
            hint.type         = token.substr(firstColon + 1, secondColon - firstColon - 1);
            hint.defaultValue = token.substr(secondColon + 1);
            if (hint.type == "bool" && (hint.defaultValue == "true" || hint.defaultValue == "True"))
                hint.defaultValue = "1";

            hintMap[keyPath] = hint;
        }
        return hintMap;
    }

    bool EnsureConfigDefaults(std::vector<ConfigEntry>& config, const std::map<std::string, ConfigHint>& hintMap) {
        bool changed = false;

        for (const auto& [keyPath, hint] : hintMap) {
            size_t slashPos = keyPath.find('/');
            if (slashPos == std::string::npos || slashPos == 0 || slashPos + 1 >= keyPath.size()) {
                continue;
            }

            const std::string section = keyPath.substr(0, slashPos);
            const std::string key = keyPath.substr(slashPos + 1);

            bool exists = false;
            for (const auto& entry : config) {
                if (_stricmp(entry.section.c_str(), section.c_str()) == 0 &&
                    _stricmp(entry.key.c_str(), key.c_str()) == 0) {
                    exists = true;
                    break;
                }
            }

            if (!exists) {
                config.push_back({ section, key, hint.defaultValue });
                changed = true;
            }
        }

        return changed;
    }

    void LoadAllConfigs() {
        const int count = 1 + (int)mods.size();
        g_allConfigs.resize(count);
        g_allHintMaps.resize(count);

        for (int i = 0; i < count; ++i) {
            const std::wstring configTypes = (i == 0) ? LOADER_CONFIG_TYPES_L : mods[i - 1].info.configTypes;
            const std::wstring configPath  = GetConfigPath(i);

            g_allHintMaps[i] = ParseConfigHints(configTypes);
            g_allConfigs[i]  = LauncherIni::ParseIniFile(configPath);

            if (EnsureConfigDefaults(g_allConfigs[i], g_allHintMaps[i])) {
                LauncherIni::WriteIniFile(configPath, g_allConfigs[i]);
            }
        }
    }

    std::vector<ConfigSectionView> BuildConfigSections(const std::vector<ConfigEntry>& entries) {
        std::vector<ConfigSectionView> sections;
        std::map<std::string, size_t> sectionIndices;

        for (size_t i = 0; i < entries.size(); ++i) {
            const std::string sectionName = entries[i].section.empty() ? "Configuration" : entries[i].section;
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
        auto& config  = g_allConfigs[g_selectedMod];
        auto& hintMap = g_allHintMaps[g_selectedMod];
        const std::vector<ConfigSectionView> sections = BuildConfigSections(config);

        for (size_t sectionIndex = 0; sectionIndex < sections.size(); ++sectionIndex) {
            const ConfigSectionView& section = sections[sectionIndex];
            std::string headerLabel = section.name + "##section" + std::to_string(sectionIndex);

            if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::BeginTable(("##sectionTable" + std::to_string(sectionIndex)).c_str(), 2)) {
                    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    for (size_t entryIndex : section.entryIndices) {
                        ConfigEntry& entry = config[entryIndex];

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextUnformatted(entry.key.c_str());

                        ImGui::TableSetColumnIndex(1);
                        char buffer[256];
                        strncpy_s(buffer, entry.value.c_str(), sizeof(buffer) - 1);

                        ImGui::PushID(static_cast<int>(entryIndex));
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        const std::string hintKey = entry.section + "/" + entry.key;
                        const auto hintIt = hintMap.find(hintKey);
                        const std::string type = (hintIt != hintMap.end()) ? hintIt->second.type : "string";

                        if (type == "bool") {
                            bool val = (entry.value == "1" || _stricmp(entry.value.c_str(), "true") == 0);
                            if (ImGui::Checkbox("##value", &val)) {
                                entry.value = val ? "1" : "0";
                                SaveConfigForMod(g_selectedMod);
                            }
                        } else if (type == "int") {
                            int val = 0;
                            if (!entry.value.empty()) {
                                try {
                                    val = std::stoi(entry.value);
                                }
                                catch (...) {
                                    val = 0;
                                }
                            }
                            if (ImGui::InputInt("##value", &val)) {
                                entry.value = std::to_string(val);
                                SaveConfigForMod(g_selectedMod);
                            }
                        } else {
                            if (ImGui::InputText("##value", buffer, sizeof(buffer))) {
                                entry.value = buffer;
                                SaveConfigForMod(g_selectedMod);
                            }
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

    void SaveConfigForMod(int modIndex) {
        LauncherIni::WriteIniFile(GetConfigPath(modIndex), g_allConfigs[modIndex]);
    }

    bool GetLoaderEnabledSetting() {
        for (const auto& entry : g_allConfigs[0]) {
            if (_stricmp(entry.section.c_str(), "Config") == 0 && _stricmp(entry.key.c_str(), "LoaderEnabled") == 0) {
                return entry.value == "1" || _stricmp(entry.value.c_str(), "true") == 0;
            }
        }
        return true;
    }

    void SetLoaderEnabledSetting(bool enabled) {
        const std::string newValue = enabled ? "1" : "0";
        for (auto& entry : g_allConfigs[0]) {
            if (_stricmp(entry.section.c_str(), "Config") == 0 && _stricmp(entry.key.c_str(), "LoaderEnabled") == 0) {
                entry.value = newValue;
                SaveConfigForMod(0);
                return;
            }
        }
        g_allConfigs[0].push_back({ "Config", "LoaderEnabled", newValue });
        SaveConfigForMod(0);
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
        }
        else {
            Mod& mod = mods[g_selectedMod - 1];
            name = WStringToString( mod.getName());
            version = WStringToString(mod.info.version);
            author = WStringToString(mod.info.author);
            description = WStringToString(mod.info.description);
            hyperlink = WStringToString(mod.info.hyperlink);
            filePath = WStringToString(mod.path.path);
            apiVersion = mod.info.apiVersion;
        }

        ImGui::Text(name.c_str());
        ImGui::Separator();
        ImGui::Spacing();

        if (!author.empty()) {
            ImGui::Text("Author: %s", author.c_str());
        }
        if (!version.empty() || apiVersion != -1) {
            ImGui::Text("Version: %s, API: v%d", version.c_str(), apiVersion);
        }
        ImGui::BeginChild("DetailsPane", ImVec2(0.0f, 100.0f), true);
        if (!description.empty()) {
            ImGui::TextWrapped("%s", description.c_str());
        }
        else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No description provided.");
        }
        ImGui::EndChild();
        if (!hyperlink.empty()) {
            ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), hyperlink.c_str());
            if (ImGui::IsItemClicked()) {
                ShellExecuteA(NULL, "open", hyperlink.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            }
        }
        if (!filePath.empty()) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "File: %s", filePath.c_str());
        }
    }

    void RenderSelectedItemConfig() {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextUnformatted("Configuration Options");
        ImGui::Spacing();

        if (g_allConfigs[g_selectedMod].empty()) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No config file found.");
        } else {
            RenderConfigSections();
        }
    }

}

namespace Launcher {

bool ShowLauncherWindow(HINSTANCE hInstance) {
    bool start_game = false;
    bool close_launcher = false;

    api->LogDebug("[Launcher] Setting up launcher window");

    if (!LauncherBackend::Initialize(hInstance, LOADER_NAME_L, minWindowWidth, minWindowHeight)) {
        api->LogError("[Launcher] Failed to initialize launcher window");
        return false;
    }

    api->LogDebug("[Launcher] Launcher window successfully created, initializing ImGui");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(LauncherBackend::GetWindowHandle());
    ImGui_ImplDX11_Init(LauncherBackend::GetDevice(), LauncherBackend::GetContext());
    
    RECT rect;
    GetClientRect(LauncherBackend::GetWindowHandle(), &rect);
    io.DisplaySize = ImVec2(
        static_cast<float>(rect.right - rect.left), 
        static_cast<float>(rect.bottom - rect.top)
    );

    api->LogDebug("[Launcher] ImGui initialized, entering main loop");

    LoadAllConfigs();

    while (!close_launcher) {
        LauncherBackend::PollMessages(close_launcher);
        if (close_launcher) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin(
            LOADER_NAME,
            nullptr,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar
        );

        float listWidth = 250.0f;
        float contentHeight = ImGui::GetContentRegionAvail().y;

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
                bool loaderEnabled = GetLoaderEnabledSetting();
                bool loaderSelected = (g_selectedMod == 0);
                ImGui::AlignTextToFramePadding();
                if (ImGui::Selectable((std::string(LOADER_NAME) + "##loader").c_str(), loaderSelected, 0, ImVec2(nameWidth, 0))) {
                    g_selectedMod = 0;
                }
                ImGui::SameLine(checkboxOffset);
                if (ImGui::Checkbox("##loaderCheckbox", &loaderEnabled)) {
                    SetLoaderEnabledSetting(loaderEnabled);
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
            RenderSelectedItemDetails();
            RenderSelectedItemConfig();
        }
        ImGui::EndChild();

        ImGui::End();

        ImGui::Render();
        const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.00f };
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

}
