// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "dllmain.h"
#include "draw.h"
#include "wnd.h"

static int (WINAPI* TrueEntryPoint)(VOID) = NULL;
static int (WINAPI* RawEntryPoint)(VOID) = NULL;

TEndScense TureEndScense = NULL;

HWND gHwnd = NULL;

int WINAPI hkEntryPoint()
{
	return TrueEntryPoint();
}

DWORD WINAPI MainThread(LPVOID arg)
{
	WaitForSingleObject(arg, INFINITE);

	WNDCLASSEX wc{ 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = DefWindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = _T("DummyWindow");
	if (RegisterClassEx(&wc) == NULL) 
	{
		printf("注册DummyWindow失败，错误代码：%u\n", GetLastError());
		return FALSE;
	}
	HWND hwnd = CreateWindowEx(WS_EX_APPWINDOW, wc.lpszClassName, _T(""), WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
		wc.hInstance, NULL);
	if (hwnd == NULL)
	{
		printf("创建DummyWindow失败，错误代码：%u\n", GetLastError());
		return FALSE;
	}
	
	IDirect3D9* pD3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (pD3d == NULL)
	{
		printf("Direct3DCreate9失败，错误代码：%u\n", GetLastError());
		return FALSE;
	}

	D3DPRESENT_PARAMETERS dpp{ 0 };
	dpp.Windowed = TRUE;
	dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	IDirect3DDevice9* pDevice;
	HRESULT hr = pD3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING, &dpp, &pDevice);
	if (FAILED(hr))
	{
		printf("pD3d->CreateDevice失败，错误代码：%u\n", GetLastError());
		pD3d->Release();
		DestroyWindow(hwnd);
		return FALSE;
	}

	void** pVtbl = *reinterpret_cast<void***>(pDevice);
	pD3d->Release();
	pDevice->Release();
	DestroyWindow(hwnd);

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
		freopen_s(&stream, "CONOUT$", "w", stderr);
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
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

