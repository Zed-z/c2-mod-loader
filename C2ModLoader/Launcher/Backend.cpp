#include "Backend.h"

#include "ModApi.h"
#include "Resource.h"
#include "backends/imgui_impl_win32.h"

#include <cstdio>
#include <d3d11.h>
#include <dxgi.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern ModApi *api;

namespace LauncherBackend {
namespace {
constexpr const wchar_t *kWindowClassName = LOADER_NAME_L;

HWND g_hwnd = nullptr;
int g_minWindowWidth = 720;
int g_minWindowHeight = 480;

bool g_comInitialized = false;
ID3D11Device *g_d3dDevice = nullptr;
ID3D11DeviceContext *g_d3dContext = nullptr;
IDXGISwapChain *g_swapChain = nullptr;
ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;

void CreateRenderTarget() {
	ID3D11Texture2D *backBuffer = nullptr;
	g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	if (backBuffer) {
		D3D11_TEXTURE2D_DESC desc;
		backBuffer->GetDesc(&desc);

		g_d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &g_mainRenderTargetView);

		D3D11_VIEWPORT vp;
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = static_cast<float>(desc.Width);
		vp.Height = static_cast<float>(desc.Height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		g_d3dContext->RSSetViewports(1, &vp);

		backBuffer->Release();
	}
}

void CleanupRenderTarget() {
	if (g_mainRenderTargetView) {
		g_mainRenderTargetView->Release();
		g_mainRenderTargetView = nullptr;
	}
}

bool CreateD3D(HWND hwnd) {
	RECT rect;
	GetClientRect(hwnd, &rect);
	UINT windowWidth = rect.right - rect.left;
	UINT windowHeight = rect.bottom - rect.top;

	if (windowWidth == 0 || windowHeight == 0) {
		api->LogDebug("[Launcher Backend] Window has zero dimensions");
		return false;
	}

	char buf[256]; // Message buffer
	std::snprintf(buf, sizeof(buf), "[Launcher Backend] Window size: %u x %u", windowWidth, windowHeight);
	api->LogDebug(buf);

	HRESULT hrCom = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	g_comInitialized = SUCCEEDED(hrCom);

	const D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
	D3D_FEATURE_LEVEL createdLevel{};

	HRESULT hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		featureLevels,
		2,
		D3D11_SDK_VERSION,
		&g_d3dDevice,
		&createdLevel,
		&g_d3dContext);

	if (FAILED(hr)) {
		std::snprintf(buf, sizeof(buf), "[Launcher Backend] Hardware device failed (0x%08X), trying WARP", hr);
		api->LogDebug(buf);

		hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_WARP,
			nullptr,
			0,
			featureLevels,
			2,
			D3D11_SDK_VERSION,
			&g_d3dDevice,
			&createdLevel,
			&g_d3dContext);
	}

	if (FAILED(hr)) {
		std::snprintf(buf, sizeof(buf), "[Launcher Backend] Both hardware and WARP device creation failed (0x%08X)", hr);
		api->LogDebug(buf);

		if (g_comInitialized) {
			CoUninitialize();
			g_comInitialized = false;
		}
		return false;
	}

	api->LogDebug("[Launcher Backend] D3D11 device created, now creating swap chain");

	IDXGIDevice *dxgiDevice = nullptr;
	IDXGIAdapter *dxgiAdapter = nullptr;
	IDXGIFactory *dxgiFactory = nullptr;

	hr = g_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgiDevice);
	if (SUCCEEDED(hr)) {
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		if (SUCCEEDED(hr)) {
			hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&dxgiFactory);
			if (SUCCEEDED(hr)) {
				DXGI_SWAP_CHAIN_DESC sd{};
				sd.BufferCount = 2;
				sd.BufferDesc.Width = windowWidth;
				sd.BufferDesc.Height = windowHeight;
				sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				sd.BufferDesc.RefreshRate.Numerator = 60;
				sd.BufferDesc.RefreshRate.Denominator = 1;
				sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				sd.OutputWindow = hwnd;
				sd.SampleDesc.Count = 1;
				sd.SampleDesc.Quality = 0;
				sd.Windowed = TRUE;
				sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

				hr = dxgiFactory->CreateSwapChain(g_d3dDevice, &sd, &g_swapChain);
			}
			if (dxgiFactory)
				dxgiFactory->Release();
		}
		if (dxgiAdapter)
			dxgiAdapter->Release();
	}
	if (dxgiDevice)
		dxgiDevice->Release();

	if (FAILED(hr)) {
		std::snprintf(buf, sizeof(buf), "[Launcher Backend] Swap chain creation failed (0x%08X)", hr);
		api->LogDebug(buf);

		if (g_d3dContext)
			g_d3dContext->Release();
		if (g_d3dDevice)
			g_d3dDevice->Release();
		g_d3dContext = nullptr;
		g_d3dDevice = nullptr;

		if (g_comInitialized) {
			CoUninitialize();
			g_comInitialized = false;
		}
		return false;
	}

	api->LogDebug("[Launcher Backend] D3D11 device created successfully");
	CreateRenderTarget();
	return true;
}

void CleanupD3D() {
	CleanupRenderTarget();
	if (g_swapChain) {
		g_swapChain->Release();
		g_swapChain = nullptr;
	}
	if (g_d3dContext) {
		g_d3dContext->Release();
		g_d3dContext = nullptr;
	}
	if (g_d3dDevice) {
		g_d3dDevice->Release();
		g_d3dDevice = nullptr;
	}
	if (g_comInitialized) {
		CoUninitialize();
		g_comInitialized = false;
	}
}

LRESULT CALLBACK BackendWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
		return TRUE;
	}

	switch (msg) {
	case WM_SIZE:
		if (g_d3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
			CleanupRenderTarget();
			g_swapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			CreateRenderTarget();
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) {
			return 0;
		}
		break;

	case WM_DESTROY:
		g_hwnd = nullptr;
		PostQuitMessage(0);
		return 0;

	case WM_GETMINMAXINFO:
		LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
		lpMMI->ptMinTrackSize.x = g_minWindowWidth;
		lpMMI->ptMinTrackSize.y = g_minWindowHeight;
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void PumpWindowMessagesForInitialization() {
	MSG msg;
	while (PeekMessage(&msg, g_hwnd, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}
} // namespace

bool Initialize(HINSTANCE hInstance, const wchar_t *windowTitle, int minWidth, int minHeight) {
	g_minWindowWidth = minWidth;
	g_minWindowHeight = minHeight;

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = BackendWndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = kWindowClassName;

	if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
		api->LogDebug("[Launcher Backend] Failed to register window class");
		return false;
	}

	api->LogDebug("[Launcher Backend] Window class registered, creating window");

	g_hwnd = CreateWindowW(
		wc.lpszClassName,
		windowTitle,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		g_minWindowWidth,
		g_minWindowHeight,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	if (!g_hwnd) {
		api->LogDebug("[Launcher Backend] Failed to create window");
		UnregisterClassW(wc.lpszClassName, hInstance);
		return false;
	}

	HICON hIcon = ExtractIconW(hInstance, L"Croc2.exe", 0);
	if (hIcon) {
		SendMessageW(g_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessageW(g_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		api->LogDebug("[Launcher Backend] Icon loaded from Croc2.exe");
	} else {
		api->LogDebug("[Launcher Backend] Failed to load icon from Croc2.exe");
	}

	api->LogDebug("[Launcher Backend] Window created, showing and pumping messages");

	ShowWindow(g_hwnd, SW_SHOWDEFAULT);
	UpdateWindow(g_hwnd);
	PumpWindowMessagesForInitialization();

	api->LogDebug("[Launcher Backend] Window initialized, creating D3D11");
	if (!CreateD3D(g_hwnd)) {
		api->LogDebug("[Launcher Backend] CreateD3D failed");
		DestroyWindow(g_hwnd);
		g_hwnd = nullptr;
		UnregisterClassW(wc.lpszClassName, hInstance);
		return false;
	}

	return true;
}

void Shutdown(HINSTANCE hInstance) {
	CleanupD3D();
	if (g_hwnd) {
		DestroyWindow(g_hwnd);
		g_hwnd = nullptr;
	}
	UnregisterClassW(kWindowClassName, hInstance);
}

bool PollMessages(bool &shouldQuit) {
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_QUIT) {
			shouldQuit = true;
		}
	}
	return !shouldQuit;
}

void BeginFrame(const float clearColor[4]) {
	if (!g_d3dContext || !g_mainRenderTargetView) {
		return;
	}
	g_d3dContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
	g_d3dContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);
}

void Present(UINT syncInterval, UINT flags) {
	if (g_swapChain) {
		g_swapChain->Present(syncInterval, flags);
	}
}

HWND GetWindowHandle() {
	return g_hwnd;
}

ID3D11Device *GetDevice() {
	return g_d3dDevice;
}

ID3D11DeviceContext *GetContext() {
	return g_d3dContext;
}

IDXGISwapChain *GetSwapChain() {
	return g_swapChain;
}

ID3D11RenderTargetView *GetRenderTargetView() {
	return g_mainRenderTargetView;
}

} // namespace LauncherBackend
