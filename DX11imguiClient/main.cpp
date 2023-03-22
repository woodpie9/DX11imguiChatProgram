// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs


// Client
#include "../woodnetBase/WinNetwork.h"
woodnet::WinNetwork* network;
#include "ClientProgram.h"
ClientProgram* client;


#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <locale>
#include <string>
#include <tchar.h>
#include <vector>


//

// Data
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

int main(int, char**);

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);




// Main code
int main(int, char**)
{
	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
	::RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Dear ImGui DirectX11 Chat Client", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(hwnd);

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
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// Load Fonts 한글 폰트 추가
	io.Fonts->AddFontFromFileTTF(R"(c:\Windows\Fonts\malgun.ttf)", 18.0f, NULL, io.Fonts->GetGlyphRangesKorean());

	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Our state
	bool show_demo_window = false;
	bool show_another_window = false;
	bool show_chatting_client = true;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	//network = new woodnet::WinNetwork();
	//client = new ClientProgram();

	// winsock2 사용 시작
	//network->Init();
	//client->init();

	bool isConnect = false;
	bool isLogin = false;
	bool isStartChat = false;
	static char nickname[128] = "";
	bool checkbox1 = false;
	static char password[128] = "";
	std::vector<std::string> chat_message;
	static char msgbox[128] = "";

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the WndProc() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Start the Dear ImGui frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);

		// 한글 출력
		//std::locale::global(std::locale("kor"));
		std::string stdstr = u8"한글한글";
		const char* str = u8"두글두글";

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin(u8"Hello, world! 한글도 출력");             // Create a window called "Hello, world!" and append into it.

			ImGui::Text(stdstr.c_str());
			ImGui::Text(str);
			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);
			ImGui::Checkbox(u8"체팅 클라이언트", &show_chatting_client);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		// 3. Show another simple window.
		if (show_another_window)
		{
			ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			ImGui::Text("Hello from another window!");
			if (ImGui::Button("Close Me"))
				show_another_window = false;
			ImGui::End();
		}

		if (show_chatting_client)
		{
			ImGui::SetNextWindowSize(ImVec2(520, 500), ImGuiCond_FirstUseEver);
			ImGui::Begin("Chatting Client");
			{
				if (!isConnect && !isLogin && !isStartChat)
				{
					const char* IPlist[] = { "127.0.0.1", "192.168.0.11", "192.168.0.33" };
					static int IP_current = 0;
					ImGui::Combo("IP", &IP_current, IPlist, IM_ARRAYSIZE(IPlist));
					ImGui::SameLine();

					if (ImGui::Button(u8"서버 접속"))
					{
						isConnect = true;
						chat_message.push_back(const_cast<char*>(IPlist[IP_current]));
					}
				}
				else if (isConnect && !isLogin && !isStartChat)
				{
					ImGui::InputTextWithHint("Nickname", "enter text here", nickname, IM_ARRAYSIZE(nickname));

					ImGui::InputTextWithHint("password (w/ hint)", "<password>", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);
					ImGui::Checkbox(u8"패드워드 확인", &checkbox1);
					ImGui::SameLine();
					if (ImGui::Button(u8"체팅 시작"))
					{
						isLogin = true;
						isStartChat = true;
						chat_message.push_back(nickname);
						chat_message.push_back(password);
					}

					if (checkbox1)
					{
						ImGui::Text(password);
					}

					if (ImGui::Button(u8"접속 종료"))
					{
						isConnect = false;
					}

				}
				else if (isConnect && isLogin && isStartChat)
				{
					static int track_item;
					static bool enable_track = true;
					static bool enable_extra_decorations = false;

					ImGui::Checkbox("Decoration", &enable_extra_decorations);
					ImGui::SameLine();
					ImGui::Checkbox("Track", &enable_track);
					//ImGui::PushItemWidth(100);


					ImGui::Text(u8"체팅 메시지 박스");


					const ImGuiWindowFlags child_flags = enable_extra_decorations ? ImGuiWindowFlags_MenuBar : 0;
					ImGui::BeginChild("chatText", ImVec2(0, ImGui::GetFontSize() * 20.0f), true, child_flags);
					if (ImGui::BeginMenuBar())
					{
						ImGui::TextUnformatted(u8"체팅방 이름");
						ImGui::EndMenuBar();
					}
					track_item = static_cast<int>(chat_message.size()) - 1;
					for (int item = 0; item < chat_message.size(); item++)
					{
						if (enable_track && item == track_item)
						{
							ImGui::TextColored(ImVec4(1, 1, 0, 1), chat_message[item].c_str());
							ImGui::SetScrollHereY(1); // 0.0f:top, 0.5f:center, 1.0f:bottom
						}
						else
						{
							ImGui::Text(chat_message[item].c_str());
						}
					}
					ImGui::EndChild();


					ImGui::InputTextWithHint(" ", "enter msg here", msgbox, IM_ARRAYSIZE(msgbox));
					ImGui::SameLine();
					if (ImGui::Button(u8"전송"))
					{
						char* tmp22 = new char[128];
						strcpy_s(tmp22, 128, msgbox);

						chat_message.push_back(tmp22);


					}

				}
				else
				{
					ImGui::Text(u8"여긴 누구 나는 어디");
				}


			}
			ImGui::End();


			// log
			//ImGui::Begin("logger");
			//{
			//	client->m_vector_msg;

			//	const ImGuiWindowFlags child_flags = ImGuiWindowFlags_MenuBar;
			//	ImGui::BeginChild("chatText", ImVec2(0, ImGui::GetFontSize() * 20.0f), true, child_flags);
			//	if (ImGui::BeginMenuBar())
			//	{
			//		ImGui::TextUnformatted(u8"체팅방 이름");
			//		ImGui::EndMenuBar();
			//	}

			//	int track_item = static_cast<int>(client->m_vector_msg.size()) - 1;

			//	for (int item = 0; item < client->m_vector_msg.size(); item++)
			//	{
			//		if (item == track_item)
			//		{
			//			ImGui::TextColored(ImVec4(1, 1, 0, 1), client->m_vector_msg[item].c_str());
			//			ImGui::SetScrollHereY(1); // 0.0f:top, 0.5f:center, 1.0f:bottom
			//		}
			//		else
			//		{
			//			ImGui::Text(client->m_vector_msg[item].c_str());
			//		}
			//	}
			//	ImGui::EndChild();
			//}
			//ImGui::End();
		}


		// Rendering
		ImGui::Render();
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(1, 0); // Present with vsync
		//g_pSwapChain->Present(0, 0); // Present without vsync
	}

	// Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	// winsock2 사용 종료
	//network->CleanUp();

	return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
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
	HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_WARP, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
		case WM_SIZE:
			if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
			{
				CleanupRenderTarget();
				g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
				CreateRenderTarget();
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
