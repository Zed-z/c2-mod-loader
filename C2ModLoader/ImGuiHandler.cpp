#include "ImGuiHandler.h"
#include "CheatsManager.h"
#include "Loader.h"
#include "ModApi.h"
#include "Utils.h"

#include <bitset>
#include <cmath>

#include <windows.h>

#include <d3d11.h>
#include <dxgi.h>

#include "MinHook.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

using std::sqrt, std::pow;

extern ModApi *api;

// Variables for GUI
std::vector<LogMessage> logMessages;
std::mutex logMutex;
ImFont *uiFont = nullptr;

bool showGui;
bool showLog;

bool showLogInfo;
bool showLogDebug;
bool showLogWarning;
bool showLogError;

bool showToastInfo;
bool showToastDebug;
bool showToastWarning;
bool showToastError;

bool showInputs;
bool showObjectList;
bool showCoords;
bool showLevelInfo;
bool showSaveSlotList;

static int logWidth = 1000;
static int logHeight = 300;

static int inputsWidth = 500;
static int inputsHeight = 200;

static int margin = 32;

// Globals for hook & ImGui state
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

WNDPROC oWndProc = nullptr;

LRESULT CALLBACK WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
		return TRUE; // ImGui consumed the input
	}

	// Pass the input to the game otherwises
	return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

// This runs in the Present hook whenever we see a new swap-chain
static void InitOrRestoreImGui(IDXGISwapChain *pSwap) {
	// First time initialization
	if (!g_ImguiInitialized) {
		// Grab the real device & context from this swap-chain
		if (SUCCEEDED(pSwap->GetDevice(__uuidof(ID3D11Device), (void **)&g_Device))) {
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
			ImGuiIO &io = ImGui::GetIO();
			io.Fonts->AddFontDefault();
			io.FontDefault = io.Fonts->Fonts.back();

			HRSRC hResource = FindResource(g_hModule, MAKEINTRESOURCE(IDR_UIFONT), RT_RCDATA);
			HGLOBAL hData = LoadResource(g_hModule, hResource);
			DWORD hDataSizeSize = SizeofResource(g_hModule, hResource);
			void *hResourceData = LockResource(hData);
			uiFont = io.Fonts->AddFontFromMemoryTTF(hResourceData, hDataSizeSize);

			if (g_hWnd && !oWndProc) {
				oWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)WndProcHook);
			}

			g_SwapChain = pSwap;
			g_ImguiInitialized = true;
		}
	}
	// Swap chain changed
	else if (pSwap != g_SwapChain) {
		// Shutdown ImGui backend
		ImGui_ImplDX11_Shutdown();

		// Release old render target, device and context
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

		// Get new device and context from new swap chain
		if (SUCCEEDED(pSwap->GetDevice(__uuidof(ID3D11Device), (void **)&g_Device))) {
			g_Device->GetImmediateContext(&g_Context);
			CreateRenderTarget(pSwap);
			ImGui_ImplDX11_Init(g_Device, g_Context);
			g_SwapChain = pSwap;
		}
	}
}

// Hook Present()
// re-init if needed
// render ImGui each frame
static HRESULT __stdcall hkPresent(IDXGISwapChain *pSwap, UINT sync, UINT flags) {
	InitOrRestoreImGui(pSwap);

	// Start the ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Draw the GUI
	ImGuiDraw();

	// Render on top of the game�s backbuffer
	ImGui::Render();
	g_Context->OMSetRenderTargets(1, &g_RenderTargetView, nullptr);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Call the real Present
	return oPresent(pSwap, sync, flags);
}

// Thread to wait for d3d11.dll, create a dummy device to locate Present, then hook it
DWORD WINAPI ImGuiInitThread(LPVOID lpParam) {

	g_hModule = (HMODULE)lpParam;

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

		std::string gamePathLog = "Game located at: " + gamePathStr;
		std::string ddrawPathLog = "ddraw.dll found at: " + DDrawPathStr;
		api->LogDebug(gamePathLog.c_str());
		api->LogDebug(ddrawPathLog.c_str());

		// Check if ddraw.dll is loaded from the game directory
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
		// Grab the Present address from vtable
		void **vtbl = *reinterpret_cast<void ***>(dswap);
		void *presentAddr = vtbl[8];

		// Remove dummy
		dswap->Release();
		ddev->Release();
		dctx->Release();
		DestroyWindow(dummyWnd);
		UnregisterClassA("Dummy", wc.hInstance);

		// Hook via MinHook
		MH_Initialize();
		MH_CreateHook(presentAddr, (void *)&hkPresent, reinterpret_cast<void **>(&oPresent));
		MH_EnableHook(presentAddr);
	}

	return 0;
}

std::deque<Toast> toastQueue;

void ImGuiShowToast(const std::string &message, const LogSeverity &severity, float duration) {
	toastQueue.push_front({message, severity, duration});
}

void RenderToasts() {

	ImGuiIO &io = ImGui::GetIO();

	float displayScale = io.DisplaySize.y / 720.0f;

	float toastWidth = 330.0f * displayScale;
	float toastMargin = 10.0f * displayScale;

	float margin = 20.0f * displayScale;
	float padding = 20.0f * displayScale;

	float originX = io.DisplaySize.x - toastWidth - margin * displayScale;
	float originY = margin * displayScale;

	float borderWidth = 2.0f * displayScale;
	float windowRounding = 8.0f * displayScale;

	float fontSize = 24.0f * displayScale;

	for (size_t i = 0; i < toastQueue.size();) {
		Toast &toast = toastQueue[i];
		toast.timeRemaining -= io.DeltaTime;

		// Fade out in the last 0.5 seconds
		float alpha = 1.0f;
		if (toast.timeRemaining < 0.5f) {
			alpha = toast.timeRemaining / 0.5f;
		}

		if (
			toast.severity == LogSeverity::Info && showToastInfo || toast.severity == LogSeverity::Debug && showToastDebug || toast.severity == LogSeverity::Warning && showToastWarning || toast.severity == LogSeverity::Error && showToastError) {
			ImVec4 toastColor;
			switch (toast.severity) {
			case LogSeverity::Info:
				toastColor = ImVec4(1, 1, 1, alpha);
				break;
			case LogSeverity::Debug:
				toastColor = ImVec4(0.5f, 0.8f, 1, alpha);
				break;
			case LogSeverity::Warning:
				toastColor = ImVec4(1, 1, 0.3f, alpha);
				break;
			case LogSeverity::Error:
				toastColor = ImVec4(1, 0.3f, 0.3f, alpha);
				break;
			}

			ImGui::SetNextWindowBgAlpha(alpha * 0.85f);
			ImGui::SetNextWindowPos(ImVec2(originX, originY), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(toastWidth, 0), ImGuiCond_Always);

			ImGuiWindowFlags flags =
				ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, borderWidth);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, windowRounding);
			ImGui::PushStyleColor(ImGuiCol_Border, toastColor);
			ImGui::Begin(("##Toast" + std::to_string(i)).c_str(), nullptr, flags);

			// Text with wrapping
			ImGui::PushFont(uiFont, fontSize);
			ImGui::PushStyleColor(ImGuiCol_Text, toastColor);
			ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionMax().x);
			ImGui::TextUnformatted(toast.message.c_str());
			ImGui::PopTextWrapPos();
			ImGui::PopStyleColor();
			ImGui::PopFont();

			float toastHeight = ImGui::GetWindowSize().y;
			ImGui::End();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();
			ImGui::PopStyleVar();

			// Add offset based on toast logHeight
			originY += toastHeight + toastMargin;
		}

		// Erase toast if needed
		if (toast.timeRemaining <= 0.0f) {
			toastQueue.erase(toastQueue.begin() + i);
		} else {
			i++;
		}
	}
}

// Display custom GUI
std::vector<MenuAction> menuActionRegistrations;

bool ImGuiRegisterMenuAction(HMODULE handle, MenuActionRegistrationFunction registration) {
	menuActionRegistrations.push_back({handle, registration});
	return true;
}

void RenderLog() {
	if (!showLog)
		return;

	// ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - logWidth * 0.5f, io.DisplaySize.y - logHeight - margin), ImGuiCond_Once);
	// ImGui::SetNextWindowSize(ImVec2((float)logWidth, (float)logHeight), ImGuiCond_Once);

	ImGui::Begin("Console Log");

	if (ImGui::Checkbox("Show Info##log_showinfo", &showLogInfo)) {
		api->WriteIniBool(L"Logging", L"Info", showLogInfo);
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Show Debug##log_showdebug", &showLogDebug)) {
		api->WriteIniBool(L"Logging", L"Debug", showLogDebug);
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Show Warnings##log_showwarning", &showLogWarning)) {
		api->WriteIniBool(L"Logging", L"Warning", showLogWarning);
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Show Errors##log_showerror", &showLogError)) {
		api->WriteIniBool(L"Logging", L"Error", showLogError);
	}

	ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	bool autoScroll = false;
	float scrollY = ImGui::GetScrollY();
	float scrollMaxY = ImGui::GetScrollMaxY();

	if (scrollY >= scrollMaxY - 1.0f)
		autoScroll = true;

	std::vector<LogMessage> logSnapshot;
	{
		std::lock_guard<std::mutex> lock(logMutex);
		logSnapshot = logMessages;
	}

	for (const LogMessage &msg : logSnapshot) {

		if (msg.severity == LogSeverity::Info && !showLogInfo)
			continue;
		if (msg.severity == LogSeverity::Debug && !showLogDebug)
			continue;
		if (msg.severity == LogSeverity::Warning && !showLogWarning)
			continue;
		if (msg.severity == LogSeverity::Error && !showLogError)
			continue;

		ImVec4 logColor;
		switch (msg.severity) {
		case LogSeverity::Info:
			logColor = ImVec4(1, 1, 1, 1);
			break;
		case LogSeverity::Debug:
			logColor = ImVec4(0.5f, 0.8f, 1, 1);
			break;
		case LogSeverity::Warning:
			logColor = ImVec4(1, 1, 0.3f, 1);
			break;
		case LogSeverity::Error:
			logColor = ImVec4(1, 0.3f, 0.3f, 1);
			break;
		}
		ImGui::PushStyleColor(ImGuiCol_Text, logColor);
		ImGui::TextUnformatted(msg.text.c_str());
		ImGui::PopStyleColor();
	}

	if (autoScroll)
		ImGui::SetScrollHereY(1.0f);

	ImGui::EndChild();
	ImGui::End();
}

void RenderInputs() {
	if (!showInputs)
		return;

	Inputs inputs = api->GetInputs();
	std::bitset<32> inputBits(inputs.raw);

	int analogStrength = api->AddressGetInt(ADDR_ANALOG_STRENGTH);

	int saveSlotOffset = api->AddressGetInt(ADDR_CURRENT_SAVE_SLOT) * ADDR_SAVE_SLOT_OFFSET;
	int controlScheme = api->AddressGetInt(ADDR_CONTROL_SCHEME_SLOT + saveSlotOffset);

	// ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f - inputsWidth * 0.5f, margin), ImGuiCond_Once);
	// ImGui::SetNextWindowSize(ImVec2((float)inputsWidth, (float)inputsHeight), ImGuiCond_Once);

	ImGui::Begin("Inputs");

	std::ostringstream ss0;
	ss0 << "Inputs: " << inputBits << " (" << inputs.raw << ")";

	std::ostringstream ss1;
	ss1 << "User input | Up: " << inputs.up << ", Down: " << inputs.down << ", Left: " << inputs.left << ", Right: " << inputs.right;

	std::ostringstream ss2;
	ss2 << "Effective  | Up: " << inputs.effectiveUp << ", Down: " << inputs.effectiveDown << ", Left: " << inputs.effectiveLeft << ", Right: " << inputs.effectiveRight;

	std::ostringstream ss3;
	ss3 << "Analog Strength: " << analogStrength;

	std::ostringstream ss4;
	ss4 << "Jump: " << inputs.jump << ", Attack: " << inputs.attack;

	std::ostringstream ss5;
	ss5 << "Flip: " << inputs.flip << ", Step Left: " << inputs.stepLeft << ", Step Right: " << inputs.stepRight;

	std::ostringstream ss6;
	ss6 << "Inv Use : " << inputs.invUse << ", Inv Left: " << inputs.invLeft << ", Inv Right: " << inputs.invRight;

	std::ostringstream ss7;
	ss7 << "Control Method: " << ControlSchemeNames[controlScheme];

	ImGui::Text(ss0.str().c_str());
	ImGui::Text(ss1.str().c_str());
	ImGui::Text(ss2.str().c_str());
	ImGui::Text(ss3.str().c_str());
	ImGui::Text(ss4.str().c_str());
	ImGui::Text(ss5.str().c_str());
	ImGui::Text(ss6.str().c_str());
	ImGui::Text(ss7.str().c_str());

	ImGui::End();
}

void RenderObjectList() {
	if (!showObjectList)
		return;

	ImGui::Begin("Object List");

	float itemWidth;

	uintptr_t rootObjectAddress = api->ResolveAddress(ADDR_ROOT_OBJ);
	uintptr_t crocObjectAddress = api->ResolveAddress(ADDR_CROC_OBJ);
	uintptr_t stratCountAddress = api->ResolveAddress(ADDR_STRAT_COUNT);
	if (
		rootObjectAddress != 0 && !IsBadReadPtr((void *)rootObjectAddress, sizeof(StratEntity)) && stratCountAddress != 0 && !IsBadReadPtr((void *)stratCountAddress, sizeof(int))) {
		int stratCount = api->AddressGetInt(stratCountAddress);

		StratEntity *rootObject = (StratEntity *)rootObjectAddress;

		if (rootObject->next != nullptr) {

			// Reverse to the beginning of the list
			StratEntity *node = rootObject->next;
			while (node->prev != nullptr) {
				node = node->prev;
			}

			// Croc object
			StratEntity *croc = nullptr;
			if (crocObjectAddress != 0 && !IsBadReadPtr((void *)crocObjectAddress, sizeof(StratEntity))) {
				croc = (StratEntity *)crocObjectAddress;
				croc = croc->next;
			}

			// Max distance to player
			StratEntity *distanceNode = node;
			int maxDistanceToPlayer = 0;
			if (croc == nullptr) {
				maxDistanceToPlayer = -1;
			} else {
				while (distanceNode != nullptr) {

					int playerDistance = static_cast<int>(std::sqrt(
						std::pow(distanceNode->newPosition.x - croc->newPosition.x, 2) + std::pow(distanceNode->newPosition.y - croc->newPosition.y, 2) + std::pow(distanceNode->newPosition.z - croc->newPosition.z, 2)));

					if (playerDistance > maxDistanceToPlayer) {
						maxDistanceToPlayer = playerDistance;
					}

					distanceNode = distanceNode->next;
				}
			}

			ImGui::Text(("Object count: " + std::to_string(stratCount)).c_str());

			while (node != nullptr) {

				int playerDistance = (croc != nullptr)
					? static_cast<int>(std::sqrt(
						  std::pow(node->newPosition.x - croc->newPosition.x, 2) + std::pow(node->newPosition.y - croc->newPosition.y, 2) + std::pow(node->newPosition.z - croc->newPosition.z, 2)))
					: 0;

				std::ostringstream ss;
				ss << "(" << std::hex << std::uppercase << (uintptr_t)node << ") ";
				ss << std::nouppercase << node->name;

				// Header colors
				float distanceModifier = maxDistanceToPlayer != -1 ? (1 - ((float)playerDistance / (float)maxDistanceToPlayer)) : 1;

				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f * distanceModifier, 0.35f * distanceModifier, 0.5f * distanceModifier, 1.0f));		 // A darker blue/gray
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f * distanceModifier, 0.45f * distanceModifier, 0.6f * distanceModifier, 1.0f)); // Slightly lighter on hover
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f * distanceModifier, 0.55f * distanceModifier, 0.7f * distanceModifier, 1.0f));	 // Even lighter when pressed

				if (ImGui::CollapsingHeader(ss.str().c_str())) {
					ImGui::Indent();

					// Position
					ImGui::Text("Position / Rotation");

					itemWidth = (ImGui::GetContentRegionAvail().x / 3 - ImGui::GetStyle().ItemSpacing.x * 16 / 3);

					ImGui::SetNextItemWidth(itemWidth);
					ImGui::InputInt((std::string("PosX##posx") + ss.str()).c_str(), &(node->newPosition.x), 100, 1000);
					ImGui::SameLine();
					ImGui::SetNextItemWidth(itemWidth);
					ImGui::InputInt((std::string("PosY##posy") + ss.str()).c_str(), &(node->newPosition.y), 100, 1000);
					ImGui::SameLine();
					ImGui::SetNextItemWidth(itemWidth);
					ImGui::InputInt((std::string("PosZ##posz") + ss.str()).c_str(), &(node->newPosition.z), 100, 1000);

					ImGui::SetNextItemWidth(itemWidth);
					ImGui::InputInt((std::string("RotX##rotx") + ss.str()).c_str(), &(node->newRotation.x), 100, 1000);
					ImGui::SameLine();
					ImGui::SetNextItemWidth(itemWidth);
					ImGui::InputInt((std::string("RotY##roty") + ss.str()).c_str(), &(node->newRotation.y), 100, 1000);
					ImGui::SameLine();
					ImGui::SetNextItemWidth(itemWidth);
					ImGui::InputInt((std::string("RotZ##rotz") + ss.str()).c_str(), &(node->newRotation.z), 100, 1000);

					ImGui::Text(("Distance from player: " + std::to_string(node->distanceToPlayer) + " (" + std::to_string(playerDistance) + ")").c_str());

					// More position info
					if (ImGui::CollapsingHeader(("More Position Info##moreposinfo" + ss.str()).c_str())) {

						// Old position
						ImGui::Text("Old Position / Rotation");

						itemWidth = (ImGui::GetContentRegionAvail().x / 3 - ImGui::GetStyle().ItemSpacing.x * 16 / 3);

						ImGui::SetNextItemWidth(itemWidth);
						ImGui::InputInt((std::string("PosX##oldposx") + ss.str()).c_str(), &(node->OldRotPos.position.x), 100, 1000);
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
						ImGui::InputInt((std::string("PosY##oldposy") + ss.str()).c_str(), &(node->OldRotPos.position.y), 100, 1000);
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
						ImGui::InputInt((std::string("PosZ##oldposz") + ss.str()).c_str(), &(node->OldRotPos.position.z), 100, 1000);

						ImGui::SetNextItemWidth(itemWidth);
						ImGui::InputInt((std::string("RotX##oldrotx") + ss.str()).c_str(), &(node->OldRotPos.rotation.x), 100, 1000);
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
						ImGui::InputInt((std::string("RotY##oldroty") + ss.str()).c_str(), &(node->OldRotPos.rotation.y), 100, 1000);
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
						ImGui::InputInt((std::string("RotZ##oldrotz") + ss.str()).c_str(), &(node->OldRotPos.rotation.z), 100, 1000);

						// Start Position
						ImGui::Text("Start Position");
						ImGui::SetNextItemWidth(itemWidth);
						ImGui::InputInt((std::string("PosX##startposx") + ss.str()).c_str(), &(node->StartRotPos.position.x), 100, 1000);
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
						ImGui::InputInt((std::string("PosY##startposy") + ss.str()).c_str(), &(node->StartRotPos.position.y), 100, 1000);
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
						ImGui::InputInt((std::string("PosZ##startposz") + ss.str()).c_str(), &(node->StartRotPos.position.z), 100, 1000);

						ImGui::SetNextItemWidth(itemWidth);
						ImGui::InputInt((std::string("RotX##startrotx") + ss.str()).c_str(), &(node->StartRotPos.rotation.x), 100, 1000);
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
						ImGui::InputInt((std::string("RotY##startroty") + ss.str()).c_str(), &(node->StartRotPos.rotation.y), 100, 1000);
						ImGui::SameLine();
						ImGui::SetNextItemWidth(itemWidth);
						ImGui::InputInt((std::string("RotZ##startrotz") + ss.str()).c_str(), &(node->StartRotPos.rotation.z), 100, 1000);
					}

					// Model
					ImGui::Text("Model Data");
					uintptr_t modelAddress = reinterpret_cast<uintptr_t>(node->model);
					if (ImGui::InputScalar((std::string("Model Address##model_addr") + ss.str()).c_str(), ImGuiDataType_U32, &modelAddress, NULL, NULL, "%X", ImGuiSliderFlags_None)) {
						node->model = reinterpret_cast<void *>(modelAddress);
					}

					uintptr_t animationAddress = reinterpret_cast<uintptr_t>(node->animation);
					if (ImGui::InputScalar((std::string("Animation Address##animation_addr") + ss.str()).c_str(), ImGuiDataType_U32, &animationAddress, NULL, NULL, "%X", ImGuiSliderFlags_None)) {
						node->animation = reinterpret_cast<void *>(animationAddress);
					}

					// Scale
					ImGui::Text("Scale");

					itemWidth = (ImGui::GetContentRegionAvail().x / 3 - ImGui::GetStyle().ItemSpacing.x * 8 / 3);

					ImGui::SetNextItemWidth(itemWidth);
					ImGui::InputInt((std::string("X##scalex") + ss.str()).c_str(), &(node->scale.x), 128, 4096);
					ImGui::SameLine();
					ImGui::SetNextItemWidth(itemWidth);
					ImGui::InputInt((std::string("Y##scaley") + ss.str()).c_str(), &(node->scale.y), 128, 4096);
					ImGui::SameLine();
					ImGui::SetNextItemWidth(itemWidth);
					ImGui::InputInt((std::string("Z##scalez") + ss.str()).c_str(), &(node->scale.z), 128, 4096);

					// Actions
					ImGui::Text("Actions");

					if (ImGui::Button((std::string("Teleport Here##tphere") + ss.str()).c_str())) {
						if (croc != nullptr) {
							node->newPosition.x = croc->newPosition.x;
							node->newPosition.y = croc->newPosition.y;
							node->newPosition.z = croc->newPosition.z;
						}
					}

					ImGui::SameLine();

					if (ImGui::Button((std::string("Teleport Start Here##tpstarthere") + ss.str()).c_str())) {
						if (croc != nullptr) {
							node->StartRotPos.position.x = croc->newPosition.x;
							node->StartRotPos.position.y = croc->newPosition.y;
							node->StartRotPos.position.z = croc->newPosition.z;
						}
					}

					if (ImGui::Button((std::string("Teleport To##tphere") + ss.str()).c_str())) {
						if (croc != nullptr) {
							croc->newPosition.x = node->newPosition.x;
							croc->newPosition.y = node->newPosition.y;
							croc->newPosition.z = node->newPosition.z;
						}
					}

					ImGui::SameLine();

					if (ImGui::Button((std::string("Teleport To Start##tpstarthere") + ss.str()).c_str())) {
						if (croc != nullptr) {
							croc->newPosition.x = node->StartRotPos.position.x;
							croc->newPosition.y = node->StartRotPos.position.y;
							croc->newPosition.z = node->StartRotPos.position.z;
						}
					}

					// Freeze
					{
						int flagMask = (1 << 4);
						bool isFlagSet = (node->flags0 & flagMask) != 0;
						if (ImGui::Checkbox(("Freeze##freezecheckbox" + ss.str()).c_str(), &isFlagSet)) {
							if (isFlagSet) {
								node->flags0 |= flagMask;
							} else {
								node->flags0 &= ~flagMask;
							}
						}
					}

					ImGui::SameLine();

					// Invisible
					{
						int flagMask = (1 << 24);
						bool isFlagSet = (node->flags0 & flagMask) != 0;
						if (ImGui::Checkbox(("Invisible##invisiblecheckbox" + ss.str()).c_str(), &isFlagSet)) {
							if (isFlagSet) {
								node->flags0 |= flagMask;
							} else {
								node->flags0 &= ~flagMask;
							}
						}
					}

					// Local Variables
					if (ImGui::CollapsingHeader(("Local Variables##localvars" + ss.str()).c_str())) {

						ImGui::Text("Flags");
						ImGui::InputInt(("Flags 0" + std::string("##flags0") + ss.str()).c_str(), &(node->flags0), 0, 0);

						for (int i = 0; i < 32; i++) {
							std::string checkboxLabel =
								(i < 10 ? " " : "") + std::to_string(i) + "##flags0flag" + std::to_string(i) + ss.str();
							int flagMask = (1 << i);
							bool isFlagSet = (node->flags0 & flagMask) != 0;

							if (ImGui::Checkbox(checkboxLabel.c_str(), &isFlagSet)) {
								if (isFlagSet) {
									node->flags0 |= flagMask;
								} else {
									node->flags0 &= ~flagMask;
								}
							}

							if ((i + 1) % 8 != 0 && i < 31) {
								ImGui::SameLine();
							}
						}

						ImGui::InputInt(("Flags 1" + std::string("##flags1") + ss.str()).c_str(), &(node->flags1), 0, 0);

						for (int i = 0; i < 32; i++) {
							std::string checkboxLabel =
								(i < 10 ? " " : "") + std::to_string(i) + "##flags1flag" + std::to_string(i) + ss.str();
							int flagMask = (1 << i);
							bool isFlagSet = (node->flags1 & flagMask) != 0;

							if (ImGui::Checkbox(checkboxLabel.c_str(), &isFlagSet)) {
								if (isFlagSet) {
									node->flags1 |= flagMask;
								} else {
									node->flags1 &= ~flagMask;
								}
							}

							if ((i + 1) % 8 != 0 && i < 31) {
								ImGui::SameLine();
							}
						}

						ImGui::Text("Local Variables");
						itemWidth = (ImGui::GetContentRegionAvail().x / 2 - ImGui::GetStyle().ItemSpacing.x * 8 / 3);
						for (int i = 0; i < LOCAL_VAR_COUNT; i++) {
							ImGui::SetNextItemWidth(itemWidth);

							std::string label =
								(i < 10 ? " " : "") + std::to_string(i) + std::string("##localvar") + std::to_string(i) + ss.str();

							ImGui::InputInt(label.c_str(), &(node->localVars->vars[i]), 0, 0);

							if (i % 2 == 0) {
								ImGui::SameLine();
							}
						}
					}

					ImGui::Unindent();
				}

				// Header colors
				ImGui::PopStyleColor(3);

				node = node->next;
			}

			if (ImGui::Button("Dump to Log##logdump")) {
				StratEntity *node = rootObject->next;

				// Reverse to the beginning of the list
				while (node->prev != nullptr) {
					node = node->prev;
				}

				std::ostringstream ss;

				int i = 0;
				while (node != nullptr) {

					ss << "- (" << std::hex << (uintptr_t)node << ") " << node->name;
					ss << "\t\tPos: " << node->newPosition.x << "," << node->newPosition.y << "," << node->newPosition.z;
					ss << "\t\tRot: " << node->newRotation.x << "," << node->newRotation.y << "," << node->newRotation.z;
					ss << "\t\tScale: " << node->scale.x << "," << node->scale.y << "," << node->scale.z;
					ss << "\t\tLocal Vars: ";
					for (int j = 0; j < LOCAL_VAR_COUNT; j++) {
						ss << node->localVars->vars[j] << (j < 19 ? ", " : "");
					}
					ss << std::endl;

					node = node->next;
				}

				std::string objectListLog = "Object List: (" + std::to_string(stratCount) + ")\n" + ss.str();
				api->LogInfo(objectListLog.c_str());
			}
		} else {
			ImGui::Text("No objects found!");
		}
	} else {
		ImGui::Text("No objects found!");
	}

	ImGui::End();
}

void RenderCoords() {
	if (!showCoords)
		return;

	ImGui::Begin("Coords");

	StratEntity *croc = api->GetEntity(ADDR_CROC_OBJ);
	if (croc == nullptr) {
		ImGui::Text("Unavailable!");
	} else {
		std::stringstream ss;
		ss << "X: " << croc->newPosition.x << std::endl;
		ss << "Y: " << croc->newPosition.y << std::endl;
		ss << "Z: " << croc->newPosition.z << std::endl;
		ss << "Rot: " << croc->newRotation.y << std::endl;
		ImGui::Text(ss.str().c_str());
	}

	ImGui::End();
}

void RenderLevelInfo() {
	if (!showLevelInfo)
		return;

	LevelInfo levelInfo = api->GetLevelInfo();

	ImGui::Begin("Level Info");
	std::stringstream ss;
	ss << "Tribe: " << levelInfo.tribe << std::endl;
	ss << "Level: " << levelInfo.level << std::endl;
	ss << "Map: " << levelInfo.map << std::endl;
	ImGui::Text(ss.str().c_str());
	ImGui::End();
}

void RenderSaveSlotList() {
	if (!showSaveSlotList)
		return;

	ImGui::Begin("Save Slot List");

	for (int i = 0; i < SAVE_SLOT_NUMBER; i++) {
		SaveSlot *slot = api->GetSaveSlot(i);
		std::string slotId = std::string("slot") + std::to_string(i);
		std::string slotName = std::string("Slot ") + std::to_string(i + 1) + " - " + slot->name;
		/*if (i == api->AddressGetInt(ADDR_CURRENT_SAVE_SLOT)) {
			slotName += " (Current)";
		}*/

		if (ImGui::CollapsingHeader((slotName + "##" + slotId).c_str())) {
			ImGui::Indent();

			const float itemWidth = 128;

			ImGui::Text("Info");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputText((std::string("Name##name") + slotId).c_str(), slot->name, 4);
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputText((std::string("Tribe##tribe") + slotId).c_str(), slot->tribe, 16);

			ImGui::Text("Stats");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Heart Pots##heartpots") + slotId).c_str(), reinterpret_cast<int *>(&slot->heartPots));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Health##health") + slotId).c_str(), reinterpret_cast<int *>(&slot->health));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Total Crystals##totalcrystals") + slotId).c_str(), reinterpret_cast<int *>(&slot->crystals));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Level Crystals##levelcrystals") + slotId).c_str(), reinterpret_cast<int *>(&slot->levelCrystals));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Golden Gobbos##goldengobbos") + slotId).c_str(), reinterpret_cast<int *>(&slot->goldenGobbos));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Jigsaw Pieces##jigsawpieces") + slotId).c_str(), reinterpret_cast<int *>(&slot->jigsawPieces));

			ImGui::Text("Inventory");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Binoculars##binoculars") + slotId).c_str(), reinterpret_cast<int *>(&slot->binoculars));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Keys##keys") + slotId).c_str(), reinterpret_cast<int *>(&slot->keys));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Purple Gummis##purplegummis") + slotId).c_str(), reinterpret_cast<int *>(&slot->purpleGummis));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Blue Gummis##bluegummis") + slotId).c_str(), reinterpret_cast<int *>(&slot->blueGummis));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Green Gummis##greengummis") + slotId).c_str(), reinterpret_cast<int *>(&slot->greenGummis));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Clockwork Gobbos##clockworkgobbos") + slotId).c_str(), reinterpret_cast<int *>(&slot->clockworkGobbos));

			ImGui::Text("Boss");
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Total Boss Hearts##totalbosshearts") + slotId).c_str(), reinterpret_cast<int *>(&slot->totalBossHearts));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::InputInt((std::string("Boss Hearts##bosshearts") + slotId).c_str(), reinterpret_cast<int *>(&slot->bossHearts));

			ImGui::Unindent();
		}
	}

	ImGui::End();
}

void RenderMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Mod Loader")) {
			if (ImGui::MenuItem("Show Log", nullptr, &showLog)) {
				api->WriteIniBool(L"GUI", L"ShowLog", showLog);
			}
			if (ImGui::BeginMenu("Logging")) {

				if (ImGui::MenuItem("Show Info", nullptr, &showLogInfo)) {
					api->WriteIniBool(L"Logging", L"Info", showLogInfo);
				}
				if (ImGui::MenuItem("Show Debug", nullptr, &showLogDebug)) {
					api->WriteIniBool(L"Logging", L"Debug", showLogDebug);
				}
				if (ImGui::MenuItem("Show Warnings", nullptr, &showLogWarning)) {
					api->WriteIniBool(L"Logging", L"Warning", showLogWarning);
				}
				if (ImGui::MenuItem("Show Errors", nullptr, &showLogError)) {
					api->WriteIniBool(L"Logging", L"Error", showLogError);
				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Toasts")) {

				if (ImGui::MenuItem("Show Info", nullptr, &showToastInfo)) {
					api->WriteIniBool(L"Toasts", L"Info", showToastInfo);
				}
				if (ImGui::MenuItem("Show Debug", nullptr, &showToastDebug)) {
					api->WriteIniBool(L"Toasts", L"Debug", showToastDebug);
				}
				if (ImGui::MenuItem("Show Warnings", nullptr, &showToastWarning)) {
					api->WriteIniBool(L"Toasts", L"Warning", showToastWarning);
				}
				if (ImGui::MenuItem("Show Errors", nullptr, &showToastError)) {
					api->WriteIniBool(L"Toasts", L"Error", showToastError);
				}

				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Show Inputs", nullptr, &showInputs)) {
				api->WriteIniBool(L"GUI", L"ShowInputs", showInputs);
			}
			if (ImGui::MenuItem("Show Object List", nullptr, &showObjectList)) {
				api->WriteIniBool(L"GUI", L"ShowObjectList", showObjectList);
			}
			if (ImGui::MenuItem("Show Coords", nullptr, &showCoords)) {
				api->WriteIniBool(L"GUI", L"ShowCoords", showCoords);
			}
			if (ImGui::MenuItem("Show Level Info", nullptr, &showLevelInfo)) {
				api->WriteIniBool(L"GUI", L"ShowLevelInfo", showLevelInfo);
			}
			if (ImGui::MenuItem("Show Save Slot List", nullptr, &showSaveSlotList)) {
				api->WriteIniBool(L"GUI", L"ShowSaveSlotList", showSaveSlotList);
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

			if (menuActionRegistrations.empty()) {
				ImGui::MenuItem("(Empty)", nullptr, nullptr, false);
			}

			for (const auto &registration : menuActionRegistrations) {
				Mod *mod = GetModByHandle(registration.handle);
				std::string category = mod ? WStringToString(mod->getName()) : "Unknown";

				if (ImGui::BeginMenu(category.c_str())) {
					MenuActionRegistration action = registration.function();

					ImGui::BeginDisabled(!action.enabled);
					if (ImGui::MenuItem(action.label != nullptr ? action.label : "")) {
						if (action.callback)
							action.callback();
					}
					if (ImGui::IsItemHovered() && action.tooltip != nullptr && action.tooltip[0] != '\0') {
						ImGui::SetTooltip(action.tooltip);
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

void ImGuiDraw() {
	ImGuiIO &io = ImGui::GetIO();

	LevelInfo levelInfo = api->GetLevelInfo();
	if (levelInfo.tribe == 0 && levelInfo.level == 0) {
		std::string labelText = std::string(LOADER_NAME " v" LOADER_VERSION) + "\n" + "Mods loaded: " + std::to_string(modsLoaded);
		char *labelTextChar = const_cast<char *>(labelText.c_str());
		ImVec2 labelSize = uiFont->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, 0.0f, labelTextChar);

		float displayScale = io.DisplaySize.y / 720.0f;
		float fontSize = 32.0f * displayScale;

		float labelX = 48.0f * displayScale;
		float labelY = io.DisplaySize.y - (48.0f + labelSize.y * 2.0f) * displayScale; // Offset for bottom alignment
		float labelStrokeWidth = 2.0f * displayScale;

		int labelOpacity = levelInfo.map == 0 ? 255 : 63;
		ImU32 labelColor = IM_COL32(254, 254, 200, labelOpacity);
		ImU32 labelStrokeColor = IM_COL32(101, 81, 24, labelOpacity);

		ImDrawList *drawList = ImGui::GetBackgroundDrawList();
		drawList->AddText(uiFont, fontSize, ImVec2(labelX - labelStrokeWidth, labelY), labelStrokeColor, labelTextChar);
		drawList->AddText(uiFont, fontSize, ImVec2(labelX + labelStrokeWidth, labelY), labelStrokeColor, labelTextChar);
		drawList->AddText(uiFont, fontSize, ImVec2(labelX, labelY - labelStrokeWidth), labelStrokeColor, labelTextChar);
		drawList->AddText(uiFont, fontSize, ImVec2(labelX, labelY + labelStrokeWidth), labelStrokeColor, labelTextChar);
		drawList->AddText(uiFont, fontSize, ImVec2(labelX - labelStrokeWidth, labelY - labelStrokeWidth), labelStrokeColor, labelTextChar);
		drawList->AddText(uiFont, fontSize, ImVec2(labelX - labelStrokeWidth, labelY + labelStrokeWidth), labelStrokeColor, labelTextChar);
		drawList->AddText(uiFont, fontSize, ImVec2(labelX + labelStrokeWidth, labelY - labelStrokeWidth), labelStrokeColor, labelTextChar);
		drawList->AddText(uiFont, fontSize, ImVec2(labelX + labelStrokeWidth, labelY + labelStrokeWidth), labelStrokeColor, labelTextChar);
		drawList->AddText(uiFont, fontSize, ImVec2(labelX, labelY), labelColor, labelTextChar);
	}

	if (showGui) {
		RenderLog();
		RenderInputs();
		RenderObjectList();
		RenderCoords();
		RenderMenuBar();
		RenderLevelInfo();
		RenderSaveSlotList();
	}

	// Toast notifications
	RenderToasts();
}
