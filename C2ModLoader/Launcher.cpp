#include "ModApi.h"
#include "Config.h"
#include "ImGuiHandler.h"
#include "MouseCaptureRemover.h"
#include "Utils.h"
#include "Resource.h"
#include "Launcher.h"
#include "Loader.h"

#include <Windows.h>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <regex>

extern ModApi* api;

std::vector<HWND> g_checkboxes;
HWND g_listView = nullptr;
static bool g_userConfirmed = false;

#include <CommCtrl.h>
#pragma comment(lib, "Comctl32.lib")

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

constexpr int launcher_margin = 10;

constexpr int launcher_launch_button_width = 180;
constexpr int launcher_launch_button_height = 50;

constexpr int launcher_window_width = 460;
constexpr int launcher_window_height = 360;
constexpr int launcher_listview_height = launcher_window_height - launcher_launch_button_height - launcher_margin * 3;

#define COL_CHECKBOX 0 // Dummy column for checkboxes so that they don't trigger the popup menu
#define COL_MOD_NAME 1
#define COL_VERSION 2
#define COL_AUTHOR 3
#define COL_CONFIGURE 4

LRESULT CALLBACK LauncherWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
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
        col.pszText = (LPWSTR)L"";
        col.cx = 20;
        col.iSubItem = COL_CHECKBOX;
        ListView_InsertColumn(g_listView, COL_CHECKBOX, &col);

        col.pszText = (LPWSTR)L"Mod (Click for more info)";
        col.cx = 180;
        col.iSubItem = COL_MOD_NAME;
        ListView_InsertColumn(g_listView, COL_MOD_NAME, &col);

        // Column: Version
        col.pszText = (LPWSTR)L"Version";
        col.cx = 60;
        col.iSubItem = COL_VERSION;
        ListView_InsertColumn(g_listView, COL_VERSION, &col);

        // Column: Author
        col.pszText = (LPWSTR)L"Author";
        col.cx = 110;
        col.iSubItem = COL_AUTHOR;
        ListView_InsertColumn(g_listView, COL_AUTHOR, &col);

        // Column: Configure
        col.cx = 80 - GetSystemMetrics(SM_CXVSCROLL);;
        col.pszText = (LPWSTR)L"Config";
        ListView_InsertColumn(g_listView, COL_CONFIGURE, &col);

        // ---------------------------------------------------------------------------------------------------------------

        for (size_t i = 0; i < mods.size(); ++i) {
            auto mod = mods[i];

            LVITEMW item = {};
            item.mask = LVIF_TEXT;
            item.iItem = (int)i;
            item.pszText = const_cast<LPWSTR>(L"");

            ListView_InsertItem(g_listView, &item);

            // Column: Version
            std::wstring modName = mod.getName();
            ListView_SetItemText(g_listView, (int)i, COL_MOD_NAME, const_cast<LPWSTR>(modName.c_str()));

            // Column: Version
            ListView_SetItemText(g_listView, (int)i, COL_VERSION, const_cast<LPWSTR>(mod.info.version.c_str()));

            // Column: Author
            ListView_SetItemText(g_listView, (int)i, COL_AUTHOR, const_cast<LPWSTR>(mod.info.author.c_str()));

            // Column: Configure
            std::wstring iniPath = mod.path.path;
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
            ListView_SetCheckState(g_listView, (int)i, mod.enabled);

        }
        SendMessageW(g_listView, WM_SETFONT, (WPARAM)hFont, TRUE);

        HWND hwndLaunch = CreateWindowExW(0, WC_BUTTONW, L"Launch Game",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            launcher_window_width / 2 - launcher_launch_button_width / 2,
            launcher_window_height - launcher_launch_button_height - launcher_margin,
            launcher_launch_button_width - launcher_margin * 2,
            launcher_launch_button_height - launcher_margin * 2,
            hwnd, (HMENU)1, hInstance, nullptr);

        SendMessageW(hwndLaunch, WM_SETFONT, (WPARAM)hFont, TRUE);

        break;
    }

    case WM_COMMAND:

        // About window
        if (LOWORD(wParam) == ID_HELP_ABOUT) {
            std::wstring message = LOADER_NAME_L
                L"\nVersion: " LOADER_VERSION_L L" (API v" API_VERSION_STR L")"
                L"\nCreated by: " AUTHOR_NAME_L
                L"\n\n" LOADER_DESCRIPTION_L
                L"\n\n" LOADER_HYPERLINK_L;
            MessageBoxW(NULL, message.c_str(), LOADER_NAME_L, MB_OK | MB_ICONINFORMATION);
        }

        // Open settings
        if (LOWORD(wParam) == ID_FILE_SETTINGS) {
            OpenNotepad(CONFIG_FILE_L);
        }

        // View log
        if (LOWORD(wParam) == ID_FILE_LOG) {
            OpenNotepad(LOG_FILE_L);
        }

        // Launch button
        if (LOWORD(wParam) == 1) {
            g_userConfirmed = true;
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

        // Handle checkbox toggle in real-time
        if (nmhdr->idFrom == 1001 && nmhdr->code == LVN_ITEMCHANGED) {
            LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

            if ((pnmv->uChanged & LVIF_STATE) &&
                ((pnmv->uNewState ^ pnmv->uOldState) & LVIS_STATEIMAGEMASK)) {

                bool checked = ListView_GetCheckState(g_listView, pnmv->iItem);
                mods[pnmv->iItem].enabled = checked;
            }
        }

        // Press list item
        if (((LPNMHDR)lParam)->idFrom == 1001 && ((LPNMHDR)lParam)->code == NM_CLICK) {
            LPNMITEMACTIVATE nmItem = (LPNMITEMACTIVATE)lParam;

            // Configure column
            if (nmItem->iItem >= 0 && nmItem->iSubItem == COL_CONFIGURE) {
                std::wstring iniPath(mods[nmItem->iItem].path.path);
                iniPath.replace(iniPath.length() - 4, 4, L".ini");
                OpenNotepad(iniPath);
            }

            // Name column
            if (nmItem->iItem >= 0 && nmItem->iSubItem == COL_MOD_NAME) {
				Mod& mod = mods[nmItem->iItem];

                std::wstring message = mod.getName()
                    + (mod.info.version.length() > 0 ? L"\nVersion: " + mod.info.version + L" (API v" + std::to_wstring(mod.info.apiVersion) + L")" : L"")
                    + (mod.info.author.length() > 0 ? L"\nCreated by: " + mod.info.author : L"")
                    + (mod.info.description.length() > 0 ? L"\n\n" + mod.info.description : L"")
                    + (mod.info.hyperlink.length() > 0 ? L"\n\n" + mod.info.hyperlink : L"")
                    + L"\n\nFile location: " + mod.path.path;
                MessageBoxW(NULL, message.c_str(), LOADER_NAME_L, MB_OK | MB_ICONINFORMATION);
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
                case COL_CHECKBOX:
                    tipText = L"";
                    break;
                case COL_MOD_NAME:
                    tipText = mods[row].getName();
                    break;
                case COL_VERSION:
                    tipText = mods[row].info.version;
                    break;
                case COL_AUTHOR:
                    tipText = mods[row].info.author;
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
        g_userConfirmed = false;
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool ShowLauncherWindow(HINSTANCE hInstance) {

    // Window
    const wchar_t CLASS_NAME[] = L"C2ModLoaderLauncher";
    WNDCLASS wc = {};
    wc.lpfnWndProc = LauncherWndProc;
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
