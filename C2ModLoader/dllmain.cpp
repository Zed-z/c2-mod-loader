#define IS_MOD_LOADER
#include "ModApi.h"
#include "Config.h"
#include "ImGuiHandler.h"
#include "MouseCaptureRemover.h"
#include "Utils.h"

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


std::vector<std::wstring> g_allMods;
std::vector<std::wstring> g_selectedMods;
std::vector<HWND> g_checkboxes;
HWND g_listView = nullptr;
static bool g_userConfirmed = false;

#include <CommCtrl.h>
#pragma comment(lib, "Comctl32.lib")

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

static DWORD WINAPI OpenNotepad(LPVOID lParam) {
    wchar_t* path = (wchar_t*)lParam;
    std::wstring pathStr(path);
    api->Log("Opening " + WStringToString(pathStr));

    if (PathFileExistsW(path)) {
        std::wstring cmd = L"notepad.exe " + pathStr;
        _wsystem(cmd.c_str());
    }

    delete[] path;
    return TRUE;
}

LRESULT CALLBACK ConfigWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {

        HINSTANCE hInstance = (HINSTANCE)((LPCREATESTRUCT)lParam)->lpCreateParams;
        INITCOMMONCONTROLSEX icex = { sizeof(icex) };
        icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES;
        InitCommonControlsEx(&icex);

        HFONT hFont = CreateFontW(
            -16, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        g_listView = CreateWindowExW(0, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_AUTOARRANGE,
            10, 10, 640-10, 300,
            hwnd, (HMENU)1001, GetModuleHandle(nullptr), nullptr);

        ListView_SetExtendedListViewStyle(g_listView, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        RECT rc;
        GetClientRect(g_listView, &rc);
        int listViewWidth = rc.right - rc.left;

        LVCOLUMNW col = { 0 };
        col.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

        // Column 0: Mod Name
        col.pszText = (LPWSTR)L"Mod Name";
        col.cx = 200;
        col.iSubItem = 0;
        ListView_InsertColumn(g_listView, 0, &col);

        // Column 1: File Path
        col.pszText = (LPWSTR)L"File Path";
        col.cx = 320;
        col.iSubItem = 1;
        ListView_InsertColumn(g_listView, 1, &col);

        // Column 2: Configure
        col.cx = 100 - GetSystemMetrics(SM_CXVSCROLL);;
        col.pszText = (LPWSTR)L"Configure";
        ListView_InsertColumn(g_listView, 2, &col);

        g_allMods = GetMods(MOD_FOLDER_L);
        for (size_t i = 0; i < g_allMods.size(); ++i) {

            PathInfo modInfo = GetPathInfo(g_allMods[i]);

            LVITEMW item = {};
            item.mask = LVIF_TEXT;
            item.iItem = (int)i;
            item.pszText = const_cast<LPWSTR>(modInfo.name.c_str());

            ListView_InsertItem(g_listView, &item);

            // Set second column (file path)
            ListView_SetItemText(g_listView, (int)i, 1, const_cast<LPWSTR>(modInfo.path.c_str()));

            // Set third column (ini)
            std::wstring iniPath = modInfo.path;
            iniPath.replace(iniPath.length() - 4, 4, L".ini");

            if (PathFileExistsW(iniPath.c_str())) {
                LVITEMW item = {};
                item.iItem = i;
                item.iSubItem = 2;
                item.mask = LVIF_TEXT;
                item.pszText = (LPWSTR)L"[Edit]";
                ListView_SetItem(g_listView, &item);
            }

            ListView_SetCheckState(g_listView, (int)i, TRUE);

        }
        SendMessageW(g_listView, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hwndLaunch = CreateWindowExW(0, WC_BUTTONW, L"Launch Game",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            80, 320, 100, 30, hwnd, (HMENU)1, hInstance, nullptr);

        SendMessageW(hwndLaunch, WM_SETFONT, (WPARAM)hFont, TRUE);

        break;
    }

    case WM_COMMAND:
        // Launch button
        if (LOWORD(wParam) == 1) {
            g_selectedMods.clear();
            for (int i = 0; i < ListView_GetItemCount(g_listView); ++i) {
                if (ListView_GetCheckState(g_listView, i)) {
                    g_selectedMods.push_back(g_allMods[i]);
                }
            }
            g_userConfirmed = true;
            PostQuitMessage(0);
        }
        break;

        // Press list item
    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->idFrom == 1001 && ((LPNMHDR)lParam)->code == NM_CLICK) {
            LPNMITEMACTIVATE nmItem = (LPNMITEMACTIVATE)lParam;

            // Configure column
            if (nmItem->iItem >= 0 && nmItem->iSubItem == 2) {

                std::wstring modPath = g_allMods[nmItem->iItem];
                std::wstring iniPath = modPath;
                iniPath.replace(iniPath.length() - 4, 4, L".ini");

                api->Log("path: " + WStringToString(iniPath) + " exists? " + std::to_string(PathFileExistsW(iniPath.c_str())));

                /*size_t len = (iniPath.length() + 1);
                wchar_t* iniPathChar = new wchar_t[len];
                wcsncpy_s(iniPathChar, len, iniPath.c_str(), len - 1);

                CreateThread(nullptr, 0, OpenNotepad, (LPVOID)iniPathChar, 0, nullptr);*/

                if (PathFileExistsW(iniPath.c_str())) {
                    std::wstring cmd = L"notepad.exe " + iniPath;
                    _wsystem(cmd.c_str());
                }
            }
        }
        break;

        // Close window button
    case WM_CLOSE:
        g_userConfirmed = false;
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool ShowBlockingConfigWindow(HINSTANCE hInstance) {

    // Windo
    const wchar_t CLASS_NAME[] = L"ModConfigListView";
    WNDCLASS wc = {};
    wc.lpfnWndProc = ConfigWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);


    // Create window
    HWND hwnd = CreateWindowEx(
        WS_EX_APPWINDOW, // Treat as main window, for taskbar icon
        CLASS_NAME, LOADER_NAME_L,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
        nullptr, nullptr, hInstance, hInstance);

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
        loaderEnabled = (bool)(api->ReadIniInt(L"Config", L"LoaderEnabled", true));
        api->WriteIniInt(L"Config", L"LoaderEnabled", (int)loaderEnabled);

        guiEnabled = (bool)(api->ReadIniInt(L"GUI", L"GuiEnabled", true));
        api->WriteIniInt(L"GUI", L"GuiEnabled", (int)guiEnabled);

        showLog = (bool)(api->ReadIniInt(L"GUI", L"ShowLog", false));
        api->WriteIniInt(L"GUI", L"ShowLog", (int)showLog);

        freeMouse = (bool)(api->ReadIniInt(L"Mouse", L"FreeMouse", true));
        api->WriteIniInt(L"Mouse", L"FreeMouse", (int)freeMouse);

        logHooks = (bool)(api->ReadIniInt(L"Mouse", L"LogHooks", false));
        api->WriteIniInt(L"Mouse", L"LogHooks", (int)logHooks);

        api->Log("Game version: " + GameVersions[api->GetGameVersion()]);

        // Quit if loader disabled
        if (!loaderEnabled) {
            api->Log("Loader disabled, quitting!");
            return TRUE;
        }

        // Load mods
        if (!ShowBlockingConfigWindow(hModule)) {
            api->Log("Exiting modloader.");
            ExitProcess(0);
        }
        LoadMods(g_selectedMods);

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
