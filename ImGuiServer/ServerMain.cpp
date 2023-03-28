// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs


// Client
#include "../woodnetBase/WinNetwork.h"
#include "ServerProgram.h"
#include "dx11Imgui.h"

woodnet::WinNetwork* network;
ServerProgram* server;
dx11Imgui* dx11_gui;

// Main code
int main(int, char**)
{
	// Window State
	bool show_demo_window = false;
	bool show_chatting_server = true;
	bool show_logger_window = true;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	network = new woodnet::WinNetwork();
	server = new ServerProgram();
	dx11_gui = new dx11Imgui();

	// winsock2 사용 시작
	network->Init();
	server->Init();
	dx11_gui->init();

	bool isConnect = false;

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


		// 화면을 갱신한다.
		dx11_gui->newframe();


		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);


		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			static float f = 0.0f;
			static int counter = 0;
			ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);
			ImGui::SetNextWindowPos(ImVec2(30, 30));
			ImGui::Begin(u8"Hello, world! 한글도 출력");             // Create a window called "Hello, world!" and append into it.

			// 한글 출력
			std::string stdstr = u8"한글한글";
			ImGui::Text(stdstr.c_str());
			ImGui::Text("This is some useful text.");					// Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);			// Edit bools storing our window open/close state
			ImGui::Checkbox(u8"체팅 클라이언트", &show_chatting_server);
			ImGui::Checkbox("logger window", &show_logger_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);	// Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color);	// Edit 3 floats representing a color

			if (ImGui::Button("Button"))								// Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / dx11_gui->get_io().Framerate, dx11_gui->get_io().Framerate);
			ImGui::End();
		}


		if (show_logger_window)
		{
			// log
			ImGui::SetNextWindowPos(ImVec2(30, 350));
			ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_Always);
			ImGui::Begin("logger");
			{
				ImGui::BeginChild("chatText", ImVec2(0, ImGui::GetFontSize() * 10.0f), true);

				if (ImGui::BeginMenuBar())
				{
					ImGui::TextUnformatted(u8"Logger");
					ImGui::EndMenuBar();
				}

				const int track_item = static_cast<int>(server->log_msg_.size()) - 1;
				for (int item = 0; item < server->log_msg_.size(); item++)
				{
					if (item == track_item)
					{
						ImGui::TextColored(ImVec4(1, 1, 0, 1), server->log_msg_[item].c_str());
						ImGui::SetScrollHereY(1); // 0.0f:top, 0.5f:center, 1.0f:bottom
					}
					else
					{
						ImGui::Text(server->log_msg_[item].c_str());
					}
				}

				ImGui::EndChild();
			}
			ImGui::End();
		}


		if (show_chatting_server)
		{
			ImGui::SetNextWindowPos(ImVec2(600, 30));
			ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Always);
			ImGui::Begin("Chatting Client");
			{
				if (!isConnect)
				{
					if (ImGui::Button(u8"서버 시작"))
					{
						server->Listen();
						isConnect = true;
					}
				}
				else if(isConnect)
				{
					ImGui::Button(u8"서버 작동중... 로그 확인");
					// network loop
					server->Update();
				}
				else
				{
					ImGui::Text(u8"여긴 누구 나는 어디... 로그창 확인!!");
				}
			
			}
			ImGui::End();
		}


		if (!dx11_gui->render())
		{
			return 0;
		}
	}

	// dx11, imgui 사용 종료
	dx11_gui->cleanup();
	// winsock2 사용 종료
	network->CleanUp();

	return 0;
}

