#include "DX11Imgui.h"

// 전역 선언, 전방선언
ID3D11Device* pd3dDevice = nullptr;
ID3D11DeviceContext* pd3dDeviceContext = nullptr;
IDXGISwapChain* pSwapChain = nullptr;
ID3D11RenderTargetView* mainRenderTargetView = nullptr;
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


bool DX11Imgui::Init()
{
	// Create application window
		//ImGui_ImplWin32_EnableDpiAwareness();
	wc_ = { sizeof(wc_), CS_CLASSDC, WndProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		L"ImGui Chatting", NULL };
	::RegisterClassExW(&wc_);
	hwnd_ = ::CreateWindowW(wc_.lpszClassName, L"Dear ImGui DirectX11 Chatting Program", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc_.hInstance, NULL);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd_))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc_.lpszClassName, wc_.hInstance);
		return false;
	}

	// Show the window
	::ShowWindow(hwnd_, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd_);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd_);
	ImGui_ImplDX11_Init(pd3dDevice, pd3dDeviceContext);

	// Load Fonts 한글 폰트 추가
	io.Fonts->AddFontFromFileTTF(R"(c:\Windows\Fonts\malgun.ttf)", 
		18.0f, NULL, io.Fonts->GetGlyphRangesKorean());

	return true;
}

bool DX11Imgui::Render() const
{
	// Rendering
	ImGui::Render();
	const float clear_color_with_alpha[4]
	= { clear_color_.x * clear_color_.w, clear_color_.y * clear_color_.w, clear_color_.z * clear_color_.w, clear_color_.w };
	pd3dDeviceContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	pd3dDeviceContext->ClearRenderTargetView(mainRenderTargetView, clear_color_with_alpha);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	pSwapChain->Present(1, 0); // Present with vsync
	//g_pSwapChain->Present(0, 0); // Present without vsync

	return true;
}

bool DX11Imgui::Newframe()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	return true;
}

bool DX11Imgui::Cleanup()
{
	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd_);
	::UnregisterClassW(wc_.lpszClassName, wc_.hInstance);

	return true;
}


bool DX11Imgui::CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void DX11Imgui::CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (pSwapChain) { pSwapChain->Release(); pSwapChain = NULL; }
	if (pd3dDeviceContext) { pd3dDeviceContext->Release(); pd3dDeviceContext = NULL; }
	if (pd3dDevice) { pd3dDevice->Release(); pd3dDevice = NULL; }
}

void DX11Imgui::CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
	pBackBuffer->Release();
}

void DX11Imgui::CleanupRenderTarget()
{
	if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = NULL; }
}


// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
		case WM_SIZE:
			if (pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
			{
				DX11Imgui:: CleanupRenderTarget();
				pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				DX11Imgui::CreateRenderTarget();
			}
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
