// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

//#pragma comment(lib, "Opengl32.lib")

typedef BOOL(WINAPI* PF_WGLSWAPBUFFERS)(HDC);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

PF_WGLSWAPBUFFERS TruewglSwapBuffers = NULL;

HWND gHwnd;
WNDPROC OWndProc = NULL;
static BOOL bInitialized = FALSE;
const char* glsl_version = "#version 130";

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_SIZE:
	{
		int wndWidth = LOWORD(lparam);
		int wndHeight = HIWORD(lparam);
		printf("width = %d, height = %d\n", wndWidth, wndHeight);
		break;
	}
	case WM_STYLECHANGED:
		break;
	case WM_DESTROY:
		// 释放ImGui
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		break;
	}

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return TRUE;
	}

	return CallWindowProc(OWndProc, hwnd, msg, wparam, lparam);
}

BOOL WINAPI hkwglSwapBuffers(HDC hdc)
{
	if (!bInitialized)
	{
		gHwnd = WindowFromDC(hdc);
#ifdef _WIN64
		OWndProc = (WNDPROC)SetWindowLongPtr(gHwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
#else
		OWndProc = (WNDPROC)SetWindowLongPtr(gHwnd, GWL_WNDPROC, (LONG_PTR)WndProc);
#endif // _WIN64

		//IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(gHwnd);

		bInitialized = TRUE;
	}

	ImGui_ImplOpenGL3_Init();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	ImGui_ImplOpenGL3_Shutdown();

	return TruewglSwapBuffers(hdc);
}

DWORD Main(LPVOID param)
{
	HMODULE hOGL;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, _T("OpenGL32.dll"), &hOGL);
	if (hOGL == NULL)
	{
		printf("获取OpenGL32.dll模块句柄失败，错误代码：%u\n", GetLastError());
		return FALSE;
	}
	TruewglSwapBuffers = (PF_WGLSWAPBUFFERS)GetProcAddress(hOGL, "wglSwapBuffers");

	if (TruewglSwapBuffers != NULL)
	{
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(LPVOID&)TruewglSwapBuffers, hkwglSwapBuffers);
		DetourTransactionCommit();
	}
	else
	{
		printf("获取wglSwapBuffers地址失败，错误代码：%u\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	if (DetourIsHelperProcess())
		return TRUE;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DetourRestoreAfterWith();

#ifdef _DEBUG
		AllocConsole();
		FILE* stream;
		freopen_s(&stream, "CONIN$", "r", stdin);
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONOUT$", "w", stderr);
#endif // _DEBUG

		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Main, hModule, 0, NULL);
		break;
	case DLL_PROCESS_DETACH:
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		if (TruewglSwapBuffers != NULL) DetourDetach(&(LPVOID&)TruewglSwapBuffers, hkwglSwapBuffers);
		DetourTransactionCommit();
		break;
	}
	return TRUE;
}

