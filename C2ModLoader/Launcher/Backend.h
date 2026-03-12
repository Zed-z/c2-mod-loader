#pragma once

#include <Windows.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;

namespace LauncherBackend {

bool Initialize(HINSTANCE hInstance, const wchar_t* windowTitle, int minWidth, int minHeight);
void Shutdown(HINSTANCE hInstance);
bool PollMessages(bool& shouldQuit);
void BeginFrame(const float clearColor[4]);
void Present(UINT syncInterval = 1, UINT flags = 0);

HWND GetWindowHandle();
ID3D11Device* GetDevice();
ID3D11DeviceContext* GetContext();
IDXGISwapChain* GetSwapChain();
ID3D11RenderTargetView* GetRenderTargetView();

}
