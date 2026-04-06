#include "Backend.h"

#include "Fonts.h"
#include "Loader.h"
#include "ModApi.h"
#include "Overlay.h"

#include <string>

#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>

#include "MinHook.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

extern ModApi *api;

typedef HRESULT(__stdcall *PresentFunction)(IDXGISwapChain *, UINT, UINT);
static PresentFunction oPresent = nullptr;

static IDXGISwapChain *g_SwapChain = nullptr;
static ID3D11Device *g_Device = nullptr;
static ID3D11DeviceContext *g_Context = nullptr;
static ID3D11RenderTargetView *g_RenderTargetView = nullptr;
static HWND g_hWnd = nullptr;
static HMODULE g_hModule = nullptr;

static bool g_ImguiInitialized = false;

// Helper function to create render target
static void CreateRenderTarget(IDXGISwapChain *pSwap) {
	if (g_RenderTargetView) {
		g_RenderTargetView->Release();
		g_RenderTargetView = nullptr;
	}
	ID3D11Texture2D *bb = nullptr;
	if (SUCCEEDED(pSwap->GetBuffer(0, IID_PPV_ARGS(&bb)))) {
		g_Device->CreateRenderTargetView(bb, nullptr, &g_RenderTargetView);
		bb->Release();
	}
}

// WndProc hook for ImGui input
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static WNDPROC oWndProc = nullptr;

static LRESULT CALLBACK WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
		return TRUE; // ImGui consumed the input
	}

	// Pass the input to the game otherwise.
	return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

// This runs in the Present hook whenever we see a new swap-chain.
static void InitOrRestoreImGui(IDXGISwapChain *pSwap) {
	// First time initialization.
	if (!g_ImguiInitialized) {
		// Grab the real device & context from this swap-chain.
		if (SUCCEEDED(pSwap->GetDevice(__uuidof(ID3D11Device), (void **)&g_Device))) {
			g_Device->GetImmediateContext(&g_Context);

			DXGI_SWAP_CHAIN_DESC sd;
			pSwap->GetDesc(&sd);
			g_hWnd = sd.OutputWindow;

			CreateRenderTarget(pSwap);

			// Set up ImGui.
			ImGui::CreateContext();
			ImGui_ImplWin32_Init(g_hWnd);
			ImGui_ImplDX11_Init(g_Device, g_Context);

			// Set fonts.
			ImGuiIO &io = ImGui::GetIO();
			io.Fonts->AddFontDefault();
			io.FontDefault = io.Fonts->Fonts.back();
			(void)Fonts::GetFontTitle();

			if (g_hWnd && !oWndProc) {
				oWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)WndProcHook);
			}

			g_SwapChain = pSwap;
			g_ImguiInitialized = true;
		}
	}
	// Swap chain changed.
	else if (pSwap != g_SwapChain) {
		// Shutdown ImGui backend.
		ImGui_ImplDX11_Shutdown();

		// Release old render target, device and context.
		if (g_RenderTargetView) {
			g_RenderTargetView->Release();
			g_RenderTargetView = nullptr;
		}
		if (g_Context) {
			g_Context->Release();
			g_Context = nullptr;
		}
		if (g_Device) {
			g_Device->Release();
			g_Device = nullptr;
		}

		// Get new device and context from new swap chain.
		if (SUCCEEDED(pSwap->GetDevice(__uuidof(ID3D11Device), (void **)&g_Device))) {
			g_Device->GetImmediateContext(&g_Context);
			CreateRenderTarget(pSwap);
			ImGui_ImplDX11_Init(g_Device, g_Context);
			g_SwapChain = pSwap;
		}
	}
}

// Hook Present()
// Re-init if needed.
// Render ImGui each frame.
static HRESULT __stdcall hkPresent(IDXGISwapChain *pSwap, UINT sync, UINT flags) {
	InitOrRestoreImGui(pSwap);

	// Start the ImGui frame.
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Draw the GUI.
	ImGuiDraw();

	// Render on top of the game's backbuffer.
	ImGui::Render();
	g_Context->OMSetRenderTargets(1, &g_RenderTargetView, nullptr);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Call the real Present.
	return oPresent(pSwap, sync, flags);
}

// Thread to wait for d3d11.dll, create a dummy device to locate Present, then hook it.
DWORD WINAPI OverlayInitThread(LPVOID lpParam) {

	g_hModule = (HMODULE)lpParam;

	// Check if dgVoodoo is present.
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

		std::string gamePathLog = "Game located at: " + gamePathStr;
		std::string ddrawPathLog = "ddraw.dll found at: " + DDrawPathStr;
		api->LogDebug(gamePathLog.c_str());
		api->LogDebug(ddrawPathLog.c_str());

		// Check if ddraw.dll is loaded from the game directory.
		if (DDrawDir != gameDir) {
			api->LogWarning("ddraw.dll is not loaded from the game directory! Custom GUI won't be displayed.");

			MessageBoxW(
				NULL,
				L"Failed to initialize GUI! Make sure you're using dgVoodoo.\n"
				"Mods will continue to work but custom GUI won't be displayed.",
				LOADER_NAME_L,
				MB_OK | MB_ICONWARNING);

			return 1;
		} else {
			api->LogDebug("ddraw.dll is loaded from the game directory. Custom GUI will be displayed.");
		}
	}

	// Wait for dgVoodoo (D3D11) to load.
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

	// Create a small dummy D3D11 device and swap-chain just to read the vtable.
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;

	// Dummy window class.
	WNDCLASSEXA wc = {
		sizeof(wc), CS_CLASSDC, DefWindowProcA, 0, 0,
		GetModuleHandleA(NULL), NULL, NULL, NULL, NULL,
		"Dummy", NULL};
	RegisterClassExA(&wc);

	HWND dummyWnd = CreateWindowA("Dummy", NULL, WS_OVERLAPPEDWINDOW,
		0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);
	sd.OutputWindow = dummyWnd;

	ID3D11Device *ddev = nullptr;
	ID3D11DeviceContext *dctx = nullptr;
	IDXGISwapChain *dswap = nullptr;
	D3D_FEATURE_LEVEL fl;

	if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
			nullptr, 0, D3D11_SDK_VERSION,
			&sd, &dswap, &ddev, &fl, &dctx))) {
		// Grab the Present address from vtable.
		void **vtbl = *reinterpret_cast<void ***>(dswap);
		void *presentAddr = vtbl[8];

		// Remove dummy.
		dswap->Release();
		ddev->Release();
		dctx->Release();
		DestroyWindow(dummyWnd);
		UnregisterClassA("Dummy", wc.hInstance);

		// Hook via MinHook.
		MH_Initialize();
		MH_CreateHook(presentAddr, (void *)&hkPresent, reinterpret_cast<void **>(&oPresent));
		MH_EnableHook(presentAddr);
	}

	return 0;
}
