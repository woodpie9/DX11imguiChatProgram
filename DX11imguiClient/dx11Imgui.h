#pragma once
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <locale>
#include <tchar.h>


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


class dx11Imgui
{
public:
	dx11Imgui();
	~dx11Imgui();

	bool init();
	bool render();
	bool newprame();
	bool clenup();
	ImGuiIO get_io() { return ImGui::GetIO(); };

private:
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	WNDCLASSEXW wc;
	HWND hwnd;

	// Data
	ID3D11Device* pd3dDevice;
	ID3D11DeviceContext* pd3dDeviceContext;
	IDXGISwapChain* pSwapChain;
	ID3D11RenderTargetView* mainRenderTargetView;

	// helper functions
	bool CreateDeviceD3D(HWND hWnd);
	void CleanupDeviceD3D();
	void CreateRenderTarget();
	void CleanupRenderTarget();


};
