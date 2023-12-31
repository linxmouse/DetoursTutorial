#include "pch.h"
#include "draw.h"
#include "dllmain.h"

static UINT wndWidth = 0;
static UINT wndHeight = 0;

WNDPROC OWndProc = NULL;
static BOOL ImguiInitialized = FALSE;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_SIZE:
		{
			wndWidth = LOWORD(lparam);
			wndHeight = HIWORD(lparam);
			printf("width = %d, height = %d\n", wndWidth, wndHeight);
			break;
		}
		case WM_DESTROY:
			// 释放ImGui
			ImGui_ImplWin32_Shutdown();
			ImGui_ImplDX9_Shutdown();
			ImGui::DestroyContext();
		default:
			break;
	}

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return TRUE;
	}

	return CallWindowProc(OWndProc, hwnd, msg, wparam, lparam);
}

HRESULT WINAPI hkEndScense(IDirect3DDevice9* pdevice)
{
	//D3DRECT rect = { 25, 25, 100, 100 };
	//pdevice->Clear(1, &rect, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 255, 1, 0), 0.0f, 0);

	if (!ImguiInitialized)
	{
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->GetGlyphRangesChineseSimplifiedCommon();
		D3DDEVICE_CREATION_PARAMETERS dcp;
		pdevice->GetCreationParameters(&dcp);
		gHwnd = dcp.hFocusWindow;
#ifdef _WIN64
		OWndProc = (WNDPROC)SetWindowLongPtr(gHwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
#else
		OWndProc = (WNDPROC)SetWindowLongPtr(gHwnd, GWL_WNDPROC, (LONG_PTR)WndProc);
#endif // _WIN64
		ImGui_ImplWin32_Init(gHwnd);
		ImguiInitialized = TRUE;
	}

	ImGui_ImplDX9_Init(pdevice);
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//TODO
	ImGuiWindowFlags windowFlags = 0;
	windowFlags |= ImGuiWindowFlags_NoTitleBar;
	windowFlags |= ImGuiWindowFlags_NoScrollbar;
	windowFlags |= ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoCollapse;
	windowFlags |= ImGuiWindowFlags_NoNav;
	windowFlags |= ImGuiWindowFlags_NoBackground;
	windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (!ImGui::Begin("MyIMGUI", NULL, windowFlags))
	{
		ImGui::End();
	}
	ImGui::Button("Button", ImVec2(40, 40));
	ImGui::Text("Hello World.");
	ImVec2 wndSize = ImGui::GetWindowSize();
	// 将窗口固定在右下角距边50
	ImGui::SetWindowPos("MyIMGUI", ImVec2(wndWidth - wndSize.x - 50, wndHeight - wndSize.y - 50));
	ImGui::End();

	ImGui::ShowDemoWindow();

	// ends the Dear ImGui frame. automatically called by Render().
	//ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	ImGui_ImplDX9_Shutdown();

	return TureEndScense(pdevice);
}