// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "dllmain.h"
#include "draw.h"
#include "wnd.h"

static int (WINAPI* TrueEntryPoint)(VOID) = NULL;
static int (WINAPI* RawEntryPoint)(VOID) = NULL;

TEndScense TureEndScense = NULL;

HWND gHwnd = NULL;
UINT gWndWidth = 0;
UINT gWndHeight = 0;

int WINAPI hkEntryPoint()
{
	return TrueEntryPoint();
}

DWORD WINAPI MainThread(LPVOID arg)
{
	WaitForSingleObject(arg, INFINITE);

	IDirect3D9* pD3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3d == NULL)
	{
		printf("Direct3DCreate9失败，错误代码：%u\n", GetLastError());
		return 0;
	}

	gHwnd = GetMainHWnd(GetCurrentProcessId());
	if (gHwnd == NULL)
	{
		printf("获取主窗口句柄失败，错误代码：%u\n", GetLastError());
		return FALSE;
	}
#ifdef _WIN64
	OriginProc = (WNDPROC)SetWindowLongPtr(RelationHwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
#else
	OWndProc = (WNDPROC)SetWindowLongPtr(gHwnd, GWL_WNDPROC, (LONG_PTR)WndProc);
#endif // _WIN64
	
	D3DPRESENT_PARAMETERS dpp{ 0 };
	dpp.hDeviceWindow = gHwnd;
	dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	dpp.Windowed = TRUE;
	IDirect3DDevice9* pDevice;
	HRESULT hr = pD3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, dpp.hDeviceWindow,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING, &dpp, &pDevice);
	if (FAILED(hr))
	{
		printf("pD3d->CreateDevice失败，错误代码：%u\n", GetLastError());
		pD3d->Release();
		return FALSE;
	}
	//printf("BackBufferWidth = %u, BackBufferHeight = %u\n", dpp.BackBufferWidth, dpp.BackBufferHeight);
	gWndWidth = dpp.BackBufferWidth;
	gWndHeight = dpp.BackBufferHeight;

	void** pVtbl = *reinterpret_cast<void***>(pDevice);
	pDevice->Release();

	// EndScene是IDirect3DDevice9第43个函数
	TureEndScense = (TEndScense)pVtbl[42];
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(LPVOID&)TureEndScense, hkEndScense);
	return DetourTransactionCommit();
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	if (DetourIsHelperProcess())
	{
		return TRUE;
	}

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DetourRestoreAfterWith();

#ifdef _DEBUG
		AllocConsole();
		FILE* stream;
		freopen_s(&stream, "CONIN$", "r", stdin);
		freopen_s(&stream, "CONOUT$", "w", stdout);
#endif // _DEBUG


		//DllMain中不能调用LoadLibrary,因此对入口函数进程拦截
		TrueEntryPoint = (int (WINAPI*)())DetourGetEntryPoint(NULL);
		RawEntryPoint = TrueEntryPoint;

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(LPVOID&)TrueEntryPoint, hkEntryPoint);
		DetourTransactionCommit();

		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, NULL);
		break;
	case DLL_PROCESS_DETACH:
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		if (TureEndScense != NULL)
		{
			DetourDetach(&(LPVOID&)TureEndScense, hkEndScense);
		}
		DetourDetach(&(LPVOID&)TrueEntryPoint, hkEntryPoint);
		DetourTransactionCommit();

		// 释放ImGui
		ImGui_ImplWin32_Shutdown();
		ImGui_ImplDX9_Shutdown();
		ImGui::DestroyContext();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

