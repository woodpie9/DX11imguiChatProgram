#pragma once
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <locale>
#include <tchar.h>


class dx11Imgui
{
public:
	dx11Imgui();
	~dx11Imgui();

	bool init();
	bool render() const;
	bool newframe();
	bool cleanup();

	// ������ ǥ�� ���� ���� Getter
	ImGuiIO get_io() { return ImGui::GetIO(); };

private:
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	WNDCLASSEXW wc;
	HWND hwnd;

	// helper functions
	bool CreateDeviceD3D(HWND hWnd);

	// WndProc ���� ����ϱ� ���Ͽ� static���� ���� (resize�� �ʿ�)
public:
	static void CleanupDeviceD3D();
	static void CreateRenderTarget();
	static void CleanupRenderTarget();

};
