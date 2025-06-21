#pragma once
#include "ModApi.h"
#include "ImGuiHandler.h"
#include "Utils.h"
#include "Loader.h"
#include "CheatsManager.h"

#include <Windows.h>

#include <d3d11.h>
#include <dxgi.h>

#include "MinHook.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"


extern ModApi* api;

// Variables for GUI
ImGuiTextBuffer logBuffer;
ImFont* toastFont = nullptr;

bool incompatibleWarningShown;

bool showGui;
bool showLog;
bool logMessages;
bool logDebug;
bool logWarnings;
bool logErrors;

static int width = 1000;
static int height = 300;
static int margin = 10;

// Globals for hook & ImGui state
typedef HRESULT(__stdcall* PresentFunction)(IDXGISwapChain*, UINT, UINT);
static PresentFunction oPresent = nullptr;

static IDXGISwapChain* g_SwapChain = nullptr;
static ID3D11Device* g_Device = nullptr;
static ID3D11DeviceContext* g_Context = nullptr;
static ID3D11RenderTargetView* g_RenderTargetView = nullptr;
static HWND g_hWnd = nullptr;

static bool g_ImguiInitialized = false;

// Helper function to create render target
static void CreateRenderTarget(IDXGISwapChain* pSwap)
{
    if (g_RenderTargetView) {
        g_RenderTargetView->Release();
        g_RenderTargetView = nullptr;
    }
    ID3D11Texture2D* bb = nullptr;
    if (SUCCEEDED(pSwap->GetBuffer(0, IID_PPV_ARGS(&bb)))) {
        g_Device->CreateRenderTargetView(bb, nullptr, &g_RenderTargetView);
        bb->Release();
    }
}

// WndProc hook for ImGui input
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

WNDPROC oWndProc = nullptr;

LRESULT CALLBACK WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
        return TRUE; // ImGui consumed the input
    }

    // Pass the input to the game otherwises
    return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

// This runs in the Present hook whenever we see a new swap-chain
static void InitOrRestoreImGui(IDXGISwapChain* pSwap)
{
    // First time or swap-chain changed?
    if (!g_ImguiInitialized || pSwap != g_SwapChain)
    {
        // If already initialized, tear it down
        if (g_ImguiInitialized) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            if (g_RenderTargetView) { g_RenderTargetView->Release(); g_RenderTargetView = nullptr; }
            g_Device = nullptr;
            g_Context = nullptr;
        }

        // Grab the real device & context from this swap-chain
        if (SUCCEEDED(pSwap->GetDevice(__uuidof(ID3D11Device), (void**)&g_Device))) {
            g_Device->GetImmediateContext(&g_Context);

            DXGI_SWAP_CHAIN_DESC sd;
            pSwap->GetDesc(&sd);
            g_hWnd = sd.OutputWindow;

            CreateRenderTarget(pSwap);

            // Set up ImGui
            ImGui::CreateContext();
            ImGui_ImplWin32_Init(g_hWnd);
            ImGui_ImplDX11_Init(g_Device, g_Context);

            // Set fonts
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontDefault();
            io.FontDefault = io.Fonts->Fonts.back();

            toastFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 24.0f);
            

            if (g_hWnd && !oWndProc) {
                oWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)WndProcHook);
            }

            g_SwapChain = pSwap;
            g_ImguiInitialized = true;
        }
    }
}


// Hook Present()
// re-init if needed
// render ImGui each frame
static HRESULT __stdcall hkPresent(IDXGISwapChain* pSwap, UINT sync, UINT flags) {
    InitOrRestoreImGui(pSwap);

    // Start the ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Draw the GUI
    ImGuiDraw();

    // Render on top of the game’s backbuffer
    ImGui::Render();
    g_Context->OMSetRenderTargets(1, &g_RenderTargetView, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // Call the real Present
    return oPresent(pSwap, sync, flags);
}

// Thread to wait for d3d11.dll, create a dummy device to locate Present, then hook it
DWORD WINAPI ImGuiInitThread(LPVOID lpParam) {

	// Check if dgVoodoo is present
    // (ddraw.dll loaded from the game directory)
    HMODULE hDDraw = GetModuleHandleA("ddraw.dll");
    HMODULE hGame = GetModuleHandleA(NULL);
    bool DDrawLoaded = (hDDraw != nullptr);

    if (DDrawLoaded) {

        char gamePath[MAX_PATH];
        GetModuleFileNameA(hGame, gamePath, MAX_PATH);
        std::string gamePathStr(gamePath);
		std::string gameDir = gamePathStr.substr(0, gamePathStr.find_last_of("\\/"));

        char DDrawPath[MAX_PATH];
        GetModuleFileNameA(hDDraw, DDrawPath, MAX_PATH);
		std::string DDrawPathStr(DDrawPath);
		std::string DDrawDir = DDrawPathStr.substr(0, DDrawPathStr.find_last_of("\\/"));

        api->LogDebug("Game located at: " + gamePathStr);
		api->LogDebug("ddraw.dll found at: " + DDrawPathStr);

		// Check if ddraw.dll is loaded from the game directory
        if (DDrawDir != gameDir) {
			api->LogWarning("ddraw.dll is not loaded from the game directory! Custom GUI won't be displayed.");

            if (!incompatibleWarningShown) {
                MessageBoxW(NULL, L"Failed to initialize GUI! Make sure you're using dgVoodoo.\nMods will continue to work but custom GUI won't be displayed.\nNote: this warning won't be shown again.", LOADER_NAME_L, MB_OK | MB_ICONWARNING);
                incompatibleWarningShown = true;
                api->WriteIniBool(L"GUI", L"IncompatibleWarningShown", incompatibleWarningShown);
            }

            return 1;
        } else {
            api->LogDebug("ddraw.dll is loaded from the game directory. Custom GUI will be displayed.");
		}

	}
    

    // Wait for dgVoodoo (D3D11) to load
    int attemptCount = 0;
    while (!GetModuleHandleA("d3d11.dll")) {
        Sleep(50);
        attemptCount++;

        if (attemptCount > 10) {
            api->LogWarning("Failed to get d3d11.dll handle for GUI, quitting!");
            MessageBoxW(NULL, L"Failed to grab d3d11.dll!\nCustom GUI won't be displayed.", LOADER_NAME_L, MB_OK | MB_ICONWARNING);

            return 1;
        }
    }

    // Create a small dummy D3D11 device and swap-chain just to read the vtable
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    // Dummy window class
    WNDCLASSEXA wc = {
        sizeof(wc), CS_CLASSDC, DefWindowProcA, 0, 0,
        GetModuleHandleA(NULL), NULL, NULL, NULL, NULL,
        "Dummy", NULL
    };
    RegisterClassExA(&wc);

    HWND dummyWnd = CreateWindowA("Dummy", NULL, WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);
    sd.OutputWindow = dummyWnd;

    ID3D11Device* ddev = nullptr;
    ID3D11DeviceContext* dctx = nullptr;
    IDXGISwapChain* dswap = nullptr;
    D3D_FEATURE_LEVEL fl;

    if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, &dswap, &ddev, &fl, &dctx)))
    {
        // Grab the Present address from vtable
        void** vtbl = *reinterpret_cast<void***>(dswap);
        void* presentAddr = vtbl[8];

        // Remove dummy
        dswap->Release(); ddev->Release(); dctx->Release();
        DestroyWindow(dummyWnd);
        UnregisterClassA("Dummy", wc.hInstance);

        // Hook via MinHook
        MH_Initialize();
        MH_CreateHook(presentAddr, &hkPresent, reinterpret_cast<void**>(&oPresent));
        MH_EnableHook(presentAddr);
    }

    return 0;
}




std::deque<Toast> toastQueue;

void ImGuiShowToast(const std::string& message, float duration) {
    toastQueue.push_front({ message, duration });
}

void RenderToasts() {

    ImGuiIO& io = ImGui::GetIO();

	float toastWidth = 330.0f;
	float toastMargin = 10.0f;

    float margin = 20.0f;
    float padding = 20.0f;

    float originX = io.DisplaySize.x - toastWidth - margin;
    float originY = margin;
    

    for (size_t i = 0; i < toastQueue.size(); ) {
        Toast& toast = toastQueue[i];
        toast.timeRemaining -= io.DeltaTime;

        // Fade out in the last 0.5 seconds
        float alpha = 1.0f;
        if (toast.timeRemaining < 0.5f) {
            alpha = toast.timeRemaining / 0.5f;
        }

        ImGui::SetNextWindowBgAlpha(alpha * 0.85f);
        ImGui::SetNextWindowPos(ImVec2(originX, originY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(toastWidth, 0), ImGuiCond_Always);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoNav
            | ImGuiWindowFlags_NoMove;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1, 1, 1, alpha));
        ImGui::Begin(("##Toast" + std::to_string(i)).c_str(), nullptr, flags);

        // Text with wrapping
        ImGui::PushFont(toastFont);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, alpha));
        ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionMax().x);
        ImGui::TextUnformatted(toast.message.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
		ImGui::PopFont();

        float toastHeight = ImGui::GetWindowSize().y;
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

		// Add offset based on toast height
        originY += toastHeight + toastMargin;

        // Erase toast if needed
        if (toast.timeRemaining <= 0.0f) {
            toastQueue.erase(toastQueue.begin() + i);
        }
        else {
            i++;
        }
    }
}

// Display custom GUI
std::vector<MenuAction> menuActionRegistrations;

bool ImGuiRegisterMenuAction(HMODULE handle, MenuActionRegistrationFunction registration) {
    menuActionRegistrations.push_back({ handle, registration });
    return true;
}

void ImGuiDraw() {
    ImGuiIO& io = ImGui::GetIO();

    if (showGui) {
        if (showLog) {
            ImGui::Begin("Console Log");

            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - width * 0.5f, io.DisplaySize.y - height - margin), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2((float)width, (float)height), ImGuiCond_Once);

            ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

            ImGui::TextUnformatted(logBuffer.begin());
            ImGui::SetScrollHereY(1.0f);

            ImGui::EndChild();

            ImGui::End();
        }

        // Main menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Mod Loader")) {
                if (ImGui::MenuItem("Show Log", nullptr, &showLog)) {
                    api->WriteIniBool(L"GUI", L"ShowLog", showLog);
                }
                if (ImGui::MenuItem("Log Messages", nullptr, &logMessages)) {
                    api->WriteIniBool(L"Logging", L"LogMessages", logMessages);
                }
                if (ImGui::MenuItem("Log Debug", nullptr, &logDebug)) {
                    api->WriteIniBool(L"Logging", L"LogDebug", logDebug);
                }
                if (ImGui::MenuItem("Log Warnings", nullptr, &logWarnings)) {
                    api->WriteIniBool(L"Logging", L"LogWarnings", logWarnings);
                }
                if (ImGui::MenuItem("Log Errors", nullptr, &logErrors)) {
                    api->WriteIniBool(L"Logging", L"LogErrors", logErrors);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Cheats")) {

                if (ImGui::MenuItem("Debug Menu", nullptr, &cheatsDebugMenu)) {
					setDebugMenu(cheatsDebugMenu);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Press [Inv Prev + Inv Next] for a debug menu.");
                }

                if (ImGui::MenuItem("Position Bar", nullptr, &cheatsPositionBar)) {
                    setPositionBar(cheatsPositionBar);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Press [F7] for position bar.");
                }

                if (ImGui::MenuItem("Invulnerability", nullptr, &cheatsInvulnerability)) {
                    setInvulnerability(cheatsInvulnerability);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Never take any damage.");
                }

                if (ImGui::MenuItem("Bonus Crystals", nullptr, &cheatsBonusCrystals)) {
                    setBonusCrystals(cheatsBonusCrystals);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Press [Inv Next + Attack] for 100 crystals.");
                }

                if (ImGui::MenuItem("Music Select", nullptr, &cheatsMusicSelect)) {
                    setMusicSelect(cheatsMusicSelect);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Unlocks music select in sound options.");
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Mods")) {

                for (const auto& registration : menuActionRegistrations) {
					Mod* mod = GetModByHandle(registration.handle);
					std::string category = WStringToString(mod->getName());

                    if (ImGui::BeginMenu(category.c_str())) {

                        MenuActionRegistration action = registration.function();

                        ImGui::BeginDisabled(!action.enabled);
                        if (ImGui::MenuItem(action.label.c_str())) {
                            if (action.callback) action.callback();
                        }
                        if (ImGui::IsItemHovered()) {
							ImGui::SetTooltip(action.tooltip.c_str());
                        }
						ImGui::EndDisabled();

                        ImGui::EndMenu();
                    }
				}

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }
    
    // Toast notifications
    RenderToasts();

}
