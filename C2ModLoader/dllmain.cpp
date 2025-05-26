#define IS_MOD_LOADER
#include "ModApi.h"
#include "Config.h"
#include "ImGuiHandler.h"
#include "MouseCaptureRemover.h"
#include "Utils.h"
#include "Resource.h"

#include <Windows.h>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>

ModApi* api;

bool loaderEnabled = true;
bool skipLauncher = false;
bool guiEnabled = true;

void ClearLog() {

    char modulePath[MAX_PATH];
    GetModuleFileNameA(GetModuleHandleA(NULL), modulePath, MAX_PATH);
    std::string path(modulePath);
    size_t pos = path.find_last_of("\\/");
    std::string logPath = path.substr(0, pos) + "\\" + LOG_FILE;

    std::ofstream log(logPath, std::ios::trunc);
    if (!log.is_open()) return;

    log.close();
}

// Function to show a popup for each loaded mod
void ShowModPopup(std::vector<std::wstring> loadedMods, std::vector<std::wstring> failedMods) {

    // Successfully loaded
    std::wstring message = L"Loaded mods:";
    for (auto& mod : loadedMods) {
        message += L"\n- " + mod;
    }

    // Failed to load
    if (failedMods.size() > 0) {
        message += L"\n\nFailed to load:";
        for (auto& mod : failedMods) {
            message += L"\n- " + mod;
        }
        message += L"\nCheck the log for details.";
    }

    // Show message box
    if (failedMods.size() > 0) {
        MessageBoxW(NULL, message.c_str(), LOADER_NAME_L, MB_OK | MB_ICONINFORMATION);
    }
}

// Function to load all .asi mods from the mods folder
std::vector<std::wstring> GetMods(std::wstring folder) {

    std::vector<std::wstring> mods;

    // Get mod files
    std::wstring modFolder = folder + L"\\*.asi";
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFile(modFolder.c_str(), &findFileData);

    // No files found
    if (hFind == INVALID_HANDLE_VALUE) return mods;

    // Add mod files
    do {
        if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring wideModName(findFileData.cFileName);
            std::wstring modPath = folder + L"\\" + wideModName;
            mods.push_back(modPath);
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);

    FindClose(hFind);
    return mods;
}

void LoadMods(std::vector<std::wstring> mods) {

    std::vector<std::wstring> loadedMods;
    std::vector<std::wstring> failedMods;

    for (std::wstring mod : mods) {
        HMODULE loaded = LoadLibraryW(mod.c_str());
        if (loaded) {
            api->Log("Loaded mod: " + WStringToString(mod));
            loadedMods.push_back(mod);
        }
        else {
            DWORD err = GetLastError();
            api->Log("Failed to load mod: " + WStringToString(mod) + " with error code: " + std::to_string(err));
            failedMods.push_back(mod);
        }
    }

    ShowModPopup(loadedMods, failedMods);
}

// Keyboard input thread
static DWORD WINAPI HotkeyThread(LPVOID) {
    RegisterHotKey(NULL, 1, 0, VK_TAB);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            switch (msg.wParam) {
            case 1: // TAB

                showLog = !showLog;
                api->WriteIniInt(L"GUI", L"ShowLog", (int)showLog);

                break;
            }
        }
    }
    return 0;
}



static std::vector<std::wstring> GetDisabledMods() {  
    std::vector<std::wstring> disabledMods;  

    constexpr int disabledModsStringLength = 65536;  
    std::unique_ptr<wchar_t[]> disabledModsWchar = std::make_unique<wchar_t[]>(disabledModsStringLength);  
    api->ReadIniString(L"Config", L"DisabledMods", L"", disabledModsWchar.get(), disabledModsStringLength);  
    std::wstring disabledModsStr(disabledModsWchar.get());  

    std::wstring token;  
    std::wstringstream wss(disabledModsStr);  
    while (std::getline(wss, token, L'|')) {  
        disabledMods.push_back(token);  
    }  

    return disabledMods;  
}

void SaveDisabledMods(std::vector<std::wstring> disabledMods) {

    std::wstring disabledModsStr;

    for (unsigned int i = 0; i < disabledMods.size(); i++) {
        disabledModsStr += disabledMods[i];
        if (i < disabledMods.size() - 1) {
            disabledModsStr += L"|";
        }
    }

    api->WriteIniString(L"Config", L"DisabledMods", disabledModsStr.c_str());

}



std::vector<std::wstring> g_allMods;
std::vector<PathInfo> g_allModPaths;
std::vector<FileVersionInfo> g_allModInfo;

std::vector<std::wstring> g_selectedMods;
std::vector<std::wstring> g_disabledMods;
std::vector<HWND> g_checkboxes;
HWND g_listView = nullptr;
static bool g_userConfirmed = false;

#include <CommCtrl.h>
#pragma comment(lib, "Comctl32.lib")

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

void OpenNotepad(std::wstring path) {
    if (PathFileExistsW(path.c_str())) {
        std::wstring cmd = L"notepad.exe " + path;
        _wsystem(cmd.c_str());
    }
}

constexpr int launcher_window_width = 1280;
constexpr int launcher_window_height = 460;
constexpr int launcher_listview_height = 380;
constexpr int launcher_margin = 10;

constexpr int launcher_launch_button_width = 200;
constexpr int launcher_launch_button_height = 60;

#define COL_MOD_NAME 0
#define COL_VERSION 1
#define COL_AUTHOR 2
#define COL_DESCRIPTION 3
#define COL_FILE_PATH 4
#define COL_CONFIGURE 5

LRESULT CALLBACK ConfigWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {

        HINSTANCE hInstance = (HINSTANCE)((LPCREATESTRUCT)lParam)->lpCreateParams;
        INITCOMMONCONTROLSEX icex = { sizeof(icex) };
        icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icex);

        HFONT hFont = CreateFontW(
            -14, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        g_listView = CreateWindowExW(0, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_AUTOARRANGE,
            launcher_margin, launcher_margin, launcher_window_width - launcher_margin, launcher_listview_height,
            hwnd, (HMENU)1001, GetModuleHandle(nullptr), nullptr);

        ListView_SetExtendedListViewStyle(g_listView,
            LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_INFOTIP);

        RECT rc;
        GetClientRect(g_listView, &rc);
        int listViewWidth = rc.right - rc.left;

        // Define columns ------------------------------------------------------------------------------------------------

        LVCOLUMNW col = { 0 };
        col.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

        // Column: Mod Name
        col.pszText = (LPWSTR)L"Mod Name";
        col.cx = 200;
        col.iSubItem = COL_MOD_NAME;
        ListView_InsertColumn(g_listView, COL_MOD_NAME, &col);

        // Column: Version
        col.pszText = (LPWSTR)L"Version";
        col.cx = 60;
        col.iSubItem = COL_VERSION;
        ListView_InsertColumn(g_listView, COL_VERSION, &col);

        // Column: Author
        col.pszText = (LPWSTR)L"Author";
        col.cx = 120;
        col.iSubItem = COL_AUTHOR;
        ListView_InsertColumn(g_listView, COL_AUTHOR, &col);

        // Column: Description
        col.pszText = (LPWSTR)L"Description";
        col.cx = 600;
        col.iSubItem = COL_DESCRIPTION;
        ListView_InsertColumn(g_listView, COL_DESCRIPTION, &col);

        // Column: File Path
        col.pszText = (LPWSTR)L"File Path";
        col.cx = 200;
        col.iSubItem = COL_FILE_PATH;
        ListView_InsertColumn(g_listView, COL_FILE_PATH, &col);

        // Column: Configure
        col.cx = 80 - GetSystemMetrics(SM_CXVSCROLL);;
        col.pszText = (LPWSTR)L"Config";
        ListView_InsertColumn(g_listView, COL_CONFIGURE, &col);

        // ---------------------------------------------------------------------------------------------------------------

        for (size_t i = 0; i < g_allMods.size(); ++i) {

            std::wstring mod = g_allMods[i];

            PathInfo pathInfo = GetPathInfo(mod);
            g_allModPaths.push_back(pathInfo);

            FileVersionInfo modInfo = GetFileVersionInfo(mod);
            g_allModInfo.push_back(modInfo);

            LVITEMW item = {};
            item.mask = LVIF_TEXT;
            item.iItem = (int)i;
            item.pszText = modInfo.productName.length() > 0
                ? const_cast<LPWSTR>(modInfo.productName.c_str())
                : const_cast<LPWSTR>(pathInfo.name.c_str());

            ListView_InsertItem(g_listView, &item);

            // Column: Version
            ListView_SetItemText(g_listView, (int)i, COL_VERSION, const_cast<LPWSTR>(modInfo.fileVersion.c_str()));

            // Column: Author
            ListView_SetItemText(g_listView, (int)i, COL_AUTHOR, const_cast<LPWSTR>(modInfo.companyName.c_str()));

            // Column: Description
            ListView_SetItemText(g_listView, (int)i, COL_DESCRIPTION, const_cast<LPWSTR>(modInfo.fileDescription.c_str()));

            // Column: File Path
            ListView_SetItemText(g_listView, (int)i, COL_FILE_PATH, const_cast<LPWSTR>(pathInfo.path.c_str()));

            // Column: Configure
            std::wstring iniPath = pathInfo.path;
            iniPath.replace(iniPath.length() - 4, 4, L".ini");

            if (PathFileExistsW(iniPath.c_str())) {
                LVITEMW item = {};
                item.iItem = i;
                item.iSubItem = COL_CONFIGURE;
                item.mask = LVIF_TEXT;
                item.pszText = (LPWSTR)L"[Edit]";
                ListView_SetItem(g_listView, &item);
            }

            // Select if not marked as disabled
            if (std::find(g_disabledMods.begin(), g_disabledMods.end(), pathInfo.path) != g_disabledMods.end()) {
                ListView_SetCheckState(g_listView, (int)i, FALSE);
            }
            else {
                ListView_SetCheckState(g_listView, (int)i, TRUE);
            }

        }
        SendMessageW(g_listView, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hwndLaunch = CreateWindowExW(0, WC_BUTTONW, L"Launch Game",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            launcher_window_width / 2 - launcher_launch_button_width / 2,
            launcher_window_height - launcher_launch_button_height - launcher_margin,
            launcher_launch_button_width,
            launcher_launch_button_height,
            hwnd, (HMENU)1, hInstance, nullptr);

        SendMessageW(hwndLaunch, WM_SETFONT, (WPARAM)hFont, TRUE);

        break;
    }

    case WM_COMMAND:

        // About window
        if (LOWORD(wParam) == ID_HELP_ABOUT) {
            std::wstring message = L"About " LOADER_NAME_L L":\n\n"
				L"Version: " LOADER_VERSION_L L"\n\n"
				LOADER_DESCRIPTION_L L"\n\n"
                L"Created by: " AUTHOR_NAME_L "\n\n";
            MessageBoxW(NULL, message.c_str(), LOADER_NAME_L, MB_OK | MB_ICONINFORMATION);
        }

        // Launch button
        if (LOWORD(wParam) == 1) {
            g_selectedMods.clear();
            g_disabledMods.clear();
            for (int i = 0; i < ListView_GetItemCount(g_listView); ++i) {
                if (ListView_GetCheckState(g_listView, i)) {
                    g_selectedMods.push_back(g_allMods[i]);
                }
                else {
                    g_disabledMods.push_back(g_allMods[i]);
                }
            }
            g_userConfirmed = true;
            SaveDisabledMods(g_disabledMods);
            PostQuitMessage(0);
        }
        break;

    case WM_NOTIFY: {

        // Prevent list view column resizing
        NMHDR* nmhdr = (NMHDR*)lParam;
        HWND hHeader = ListView_GetHeader(g_listView);
        if (nmhdr->hwndFrom == hHeader) {
            if (nmhdr->code == HDN_BEGINTRACKW || nmhdr->code == HDN_BEGINTRACKA) {
                return TRUE; // Block resizing
            }
        }

        // Press list item
        if (((LPNMHDR)lParam)->idFrom == 1001 && ((LPNMHDR)lParam)->code == NM_CLICK) {
            LPNMITEMACTIVATE nmItem = (LPNMITEMACTIVATE)lParam;

            // Configure column
            if (nmItem->iItem >= 0 && nmItem->iSubItem == COL_CONFIGURE) {

                std::wstring modPath = g_allMods[nmItem->iItem];
                std::wstring iniPath = modPath;
                iniPath.replace(iniPath.length() - 4, 4, L".ini");

				OpenNotepad(iniPath);
            }
        }

        // List view column tooltips
        if (((LPNMHDR)lParam)->idFrom == 1001) {
            if (((LPNMHDR)lParam)->code == LVN_GETINFOTIP) {
                NMLVGETINFOTIP* pInfoTip = (NMLVGETINFOTIP*)lParam;
                int row = pInfoTip->iItem;
                int col = pInfoTip->iSubItem;

                // Provide the full text for the hovered cell
                std::wstring tipText;
                switch (col) {
                case COL_MOD_NAME:
                    tipText = g_allModInfo[row].productName.length() > 0
                        ? g_allModInfo[row].productName
                        : g_allModPaths[row].name;
                    break;
                case COL_VERSION:
                    tipText = g_allModInfo[row].fileVersion;
                    break;
                case COL_AUTHOR:
                    tipText = g_allModInfo[row].companyName;
                    break;
                case COL_DESCRIPTION:
                    tipText = g_allModInfo[row].fileDescription;
                    break;
                case COL_FILE_PATH:
                    tipText = g_allModPaths[row].path;
                    break;
                default:
                    tipText = L"";
                }

                // Copy to the tooltip buffer
                wcsncpy_s(pInfoTip->pszText, pInfoTip->cchTextMax, tipText.c_str(), _TRUNCATE);
            }
        }

        break;
    }

        // Close window button
    case WM_CLOSE:
        g_selectedMods.clear();
        g_disabledMods.clear();
        for (int i = 0; i < ListView_GetItemCount(g_listView); ++i) {
            if (ListView_GetCheckState(g_listView, i)) {
                g_selectedMods.push_back(g_allMods[i]);
            }
            else {
                g_disabledMods.push_back(g_allMods[i]);
            }
        }
        g_userConfirmed = false;
        SaveDisabledMods(g_disabledMods);
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool ShowBlockingConfigWindow(HINSTANCE hInstance) {

    // Window
    const wchar_t CLASS_NAME[] = L"C2ModLoaderLauncher";
    WNDCLASS wc = {};
    wc.lpfnWndProc = ConfigWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);


    // Create window
    RECT rc = { 0, 0, launcher_window_width, launcher_window_height };
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    HWND hwnd = CreateWindowEx(
        WS_EX_APPWINDOW, // Treat as main window, for taskbar icon
        CLASS_NAME, LOADER_NAME_L,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr,
        LoadMenu(hInstance, MAKEINTRESOURCE(MENU_BAR)),
        hInstance,
        hInstance
    );

    if (!hwnd) return false;


    // Change the window's icon
    HICON hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    WCHAR exePath[MAX_PATH];
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH)) {
        hIcon = ExtractIconW(nullptr, exePath, 0);
    }

    if (hIcon) {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }


    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);


    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(hwnd);
    return g_userConfirmed;
}


// Entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: {
        api = GetModApi();
        ClearLog();

        // Config
        loaderEnabled = api->ReadIniBool(L"Config", L"LoaderEnabled", true);
        api->WriteIniBool(L"Config", L"LoaderEnabled", loaderEnabled);

        skipLauncher = api->ReadIniBool(L"Config", L"SkipLauncher", false);
        api->WriteIniBool(L"Config", L"SkipLauncher", skipLauncher);

        guiEnabled = api->ReadIniBool(L"GUI", L"GuiEnabled", true);
        api->WriteIniBool(L"GUI", L"GuiEnabled", guiEnabled);

        showLog = api->ReadIniBool(L"GUI", L"ShowLog", false);
        api->WriteIniBool(L"GUI", L"ShowLog", showLog);

        freeMouse = api->ReadIniBool(L"Mouse", L"FreeMouse", true);
        api->WriteIniBool(L"Mouse", L"FreeMouse", freeMouse);

        logHooks = api->ReadIniBool(L"Mouse", L"LogHooks", false);
        api->WriteIniBool(L"Mouse", L"LogHooks", logHooks);

        api->Log("Game version: " + GameVersions[api->GetGameVersion()]);

        // Quit if loader disabled
        if (!loaderEnabled) {
            api->Log("Loader disabled, quitting!");
            return TRUE;
        }

        // Initialize mod lists
        g_allMods = GetMods(MOD_FOLDER_L);
        g_disabledMods = GetDisabledMods();

        // Show launcher window
        if (!skipLauncher) {

            if (!ShowBlockingConfigWindow(hModule)) {
                api->Log("Exiting modloader.");
                ExitProcess(0);
            }
            
            // Load mods
            LoadMods(g_selectedMods);
        }
        else {

            // Load mods
            g_selectedMods.clear();
            for (auto mod : g_allMods) {
                if (std::find(g_disabledMods.begin(), g_disabledMods.end(), mod) == g_disabledMods.end()) {
                    g_selectedMods.push_back(mod);
                }
            }
            LoadMods(g_selectedMods);
        }

        // Call other components
        DisableThreadLibraryCalls(hModule);

        if (guiEnabled) {
            CreateThread(nullptr, 0, ImGuiInitThread, api, 0, nullptr);
        }

        if (freeMouse) {
            CreateThread(nullptr, 0, MouseInitThread, api, 0, nullptr);
        }

        CreateThread(nullptr, 0, HotkeyThread, nullptr, 0, nullptr);
    }
    }
    return TRUE;
}
