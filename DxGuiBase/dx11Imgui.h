#pragma once
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#include <d3d11.h>
#include <locale>
#include <tchar.h>


//	20230407 코딩스타일 적용, 사용 주석 작성
//	20230323 DX11 Imgui 분리 프로젝트 생성
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

	// 프레임 표시 등을 위한 Getter
	ImGuiIO GetIo() { return ImGui::GetIO(); };

private:
	ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	WNDCLASSEXW wc_;
	HWND hwnd_;

	// helper functions
	bool CreateDeviceD3D(HWND hWnd);

	// WndProc 에서 사용하기 위하여 static으로 변경 (resize에 필요)
public:
	static void CleanupDeviceD3D();
	static void CreateRenderTarget();
	static void CleanupRenderTarget();

};
