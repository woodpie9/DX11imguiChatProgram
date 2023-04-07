#pragma once
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <locale>
#include <tchar.h>


//	20230407 �ڵ���Ÿ�� ����, ��� �ּ� �ۼ�
//	20230323 DX11 Imgui �и� ������Ʈ ����
//
class DX11Imgui
{
public:
	DX11Imgui();
	~DX11Imgui();

	bool Init();
	bool Render() const;
	bool Newframe();
	bool Cleanup();

	// ������ ǥ�� ���� ���� Getter
	ImGuiIO GetIo() { return ImGui::GetIO(); };

private:
	ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	WNDCLASSEXW wc_;
	HWND hwnd_;

	// helper functions
	bool CreateDeviceD3D(HWND hWnd);

	// WndProc ���� ����ϱ� ���Ͽ� static���� ���� (resize�� �ʿ�)
public:
	static void CleanupDeviceD3D();
	static void CreateRenderTarget();
	static void CleanupRenderTarget();

};
