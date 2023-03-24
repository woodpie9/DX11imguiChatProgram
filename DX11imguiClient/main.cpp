// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs


// Client
#include "../woodnetBase/WinNetwork.h"
#include "ClientProgram.h"
woodnet::WinNetwork* network;
ClientProgram* client;

#include "dx11Imgui.h"
dx11Imgui* dx11_imgui;


static const char* connection_status_str[] = { " None",	"Oppend",	"SetEvent",	"Connecting",	"Connected",	"OnChat",	"Disconnected",	"Closed" };


// Main code
int main(int, char**)
{
	// Our state
	bool show_demo_window = false;
	bool show_another_window = false;
	bool show_chatting_client = true;
	bool show_logger_window = true;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	network = new woodnet::WinNetwork();
	client = new ClientProgram();
	dx11_imgui = new dx11Imgui();

	// winsock2 사용 시작
	network->Init();
	client->init();
	dx11_imgui->init();


	bool isConnect = false;
	bool checkbox1 = false;
	static char nickname[128] = "";
	static char password[128] = "";
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


		dx11_imgui->newframe();

		// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);



		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin(u8"Hello, world! 한글도 출력");             // Create a window called "Hello, world!" and append into it.

			// 한글 출력
			std::string stdstr = u8"한글한글";
			ImGui::Text(stdstr.c_str());
			ImGui::Text("This is some useful text.");					// Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);			// Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);
			ImGui::Checkbox(u8"체팅 클라이언트", &show_chatting_client);
			ImGui::Checkbox("logger window", &show_logger_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);	// Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float*)&clear_color);	// Edit 3 floats representing a color

			if (ImGui::Button("Button"))								// Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / dx11_imgui->get_io().Framerate, dx11_imgui->get_io().Framerate);
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
			if (isConnect == true)
			{
				// cliend loop
				client->network_update();
			}
			ImGui::SetNextWindowSize(ImVec2(520, 500), ImGuiCond_FirstUseEver);
			ImGui::Begin("Chatting Client");
			{
				if (!isConnect)
				{
					const char* IPlist[] = { "127.0.0.1", "192.168.0.11", "192.168.0.33" };
					static int IP_current = 0;
					ImGui::Combo("IP", &IP_current, IPlist, IM_ARRAYSIZE(IPlist));
					ImGui::SameLine();

					if (ImGui::Button(u8"서버 접속"))
					{
						isConnect = true;
						client->m_log_msg.push_back(const_cast<char*>(IPlist[IP_current]));
						client->connect_server(IPlist[IP_current]);
					}
				}
				else if (isConnect && client->get_connection_status() == ConnectionStatus::Connected)
				{
					ImGui::InputTextWithHint("Nickname", "enter text here", nickname, IM_ARRAYSIZE(nickname));

					//ImGui::InputTextWithHint("password (w/ hint)", "<password>", password, IM_ARRAYSIZE(password), ImGuiInputTextFlags_Password);
					//ImGui::Checkbox(u8"패드워드 확인", &checkbox1);
					ImGui::SameLine();
					if (ImGui::Button(u8"체팅 시작"))
					{
						client->login_server();
						client->m_log_msg.push_back(nickname);
						client->m_log_msg.push_back(password);
						client->set_nickname(nickname);
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
				else if (isConnect && client->get_connection_status() == ConnectionStatus::OnChat)
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
					track_item = static_cast<int>(client->m_vector_msg.size()) - 1;
					for (int item = 0; item < client->m_vector_msg.size(); item++)
					{
						if (enable_track && item == track_item)
						{
							ImGui::TextColored(ImVec4(1, 1, 0, 1), client->m_vector_msg[item].c_str());
							ImGui::SetScrollHereY(1); // 0.0f:top, 0.5f:center, 1.0f:bottom
						}
						else
						{
							ImGui::Text(client->m_vector_msg[item].c_str());
						}
					}
					ImGui::EndChild();


					ImGui::InputTextWithHint(" ", "enter msg here", msgbox, IM_ARRAYSIZE(msgbox));
					ImGui::SameLine();
					if (ImGui::Button(u8"전송"))
					{
						//char* tmp22 = new char[128];
						//strcpy_s(tmp22, 128, msgbox);

						//client->m_vector_msg.push_back(tmp22);

						client->send_chat_msg(msgbox);
					}

				}
				else
				{
					ImGui::Text(u8"여긴 누구 나는 어디");
				}


			}
			ImGui::End();


			// log
			ImGui::Begin("logger");
			{
				client->m_log_msg;

				const ImGuiWindowFlags child_flags = ImGuiWindowFlags_MenuBar;
				ImGui::BeginChild("chatText", ImVec2(0, ImGui::GetFontSize() * 10.0f), true, child_flags);
				if (ImGui::BeginMenuBar())
				{
					ImGui::TextUnformatted(u8"체팅방 이름");
					ImGui::EndMenuBar();
				}

				int track_item = static_cast<int>(client->m_log_msg.size()) - 1;

				for (int item = 0; item < client->m_log_msg.size(); item++)
				{
					if (item == track_item)
					{
						ImGui::TextColored(ImVec4(1, 1, 0, 1), client->m_log_msg[item].c_str());
						ImGui::SetScrollHereY(1); // 0.0f:top, 0.5f:center, 1.0f:bottom
					}
					else
					{
						ImGui::Text(client->m_log_msg[item].c_str());
					}
				}

				ImGui::EndChild();

				ConnectionStatus connection = client->get_connection_status();

				ImGui::Text(connection_status_str[(static_cast<int>(connection))]);
			}
			ImGui::End();
		}


		dx11_imgui->render();
	}

	dx11_imgui->cleanup();

	// winsock2 사용 종료
	network->CleanUp();

	return 0;
}

