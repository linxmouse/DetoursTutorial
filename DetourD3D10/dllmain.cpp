// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

typedef HRESULT(STDMETHODCALLTYPE* PF_Present)(
	IDXGISwapChain* pSwapChain,
	UINT syncInterval, UINT flags);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND gHwnd;
static BOOL bInitialized = FALSE;
static IDXGISwapChain* pSwapChain;
static ID3D10Device* pDevice;

static int (WINAPI* TrueEntryPoint)(VOID) = NULL;
static int (WINAPI* RawEntryPoint)(VOID) = NULL;

PF_Present TruePresent = NULL;

WNDPROC OWndProc = NULL;

int WINAPI hkEntryPoint()
{
	return TrueEntryPoint();
}

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
		ImGui_ImplDX10_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		// 释放设备
		if (pDevice) pDevice->Release();
		pSwapChain->Release();
		break;
	}

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return TRUE;
	}

	return CallWindowProc(OWndProc, hwnd, msg, wparam, lparam);
}

HRESULT STDMETHODCALLTYPE hkPresent(IDXGISwapChain* lpSwapChain, UINT SyncInterval, UINT Flags)
{
	pSwapChain = lpSwapChain;
	HRESULT hr = lpSwapChain->GetDevice(__uuidof(ID3D10Device), (LPVOID*)&pDevice);
	if (FAILED(hr)) return TruePresent(lpSwapChain, SyncInterval, Flags);

	if (!bInitialized)
	{
		DXGI_SWAP_CHAIN_DESC sd;
		lpSwapChain->GetDesc(&sd);
		gHwnd = sd.OutputWindow;

#ifdef _WIN64
		OWndProc = (WNDPROC)SetWindowLongPtr(gHwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
#else
		OWndProc = (WNDPROC)SetWindowLongPtr(gHwnd, GWL_WNDPROC, (LONG_PTR)WndProc);
#endif // _WIN64

		ImGui::CreateContext();
		ImGui_ImplWin32_Init(gHwnd);

		bInitialized = TRUE;
	}

	ImGui_ImplDX10_Init(pDevice);
	ImGui_ImplDX10_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();
	ImGui::SetNextWindowSize(ImVec2(200, 100));
	ImGui::Text("Hello World.");
	if (ImGui::ColorButton("ClickMe", ImVec4(1, 0, 0, 1), 0, ImVec2(30, 60)))
	{
		MessageBox(NULL, _T("Button Clicked!"), _T(""), MB_OK);
	}

	ImGui::Render();
	ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());

	// 释放
	ImGui_ImplDX10_Shutdown();
	pDevice->Release();
	pDevice = NULL;
	if (pSwapChain != lpSwapChain)
	{
		pSwapChain->Release();
		pSwapChain = lpSwapChain;
	}

	return TruePresent(lpSwapChain, SyncInterval, Flags);
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
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		printf("创建DummyWindows失败，错误代码：%u\n", GetLastError());
		return FALSE;
	}

	DXGI_SWAP_CHAIN_DESC scd{ 0 };
	scd.BufferCount = 1;
	scd.BufferDesc.Width = 2;
	scd.BufferDesc.Height = 2;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.RefreshRate.Numerator = 60;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.OutputWindow = hwnd;
	scd.Windowed = TRUE;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.Flags = 0;

	IDXGISwapChain* swapChain;
	ID3D10Device* device;
	HRESULT hr = D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_REFERENCE, NULL,
		0, D3D10_SDK_VERSION, &scd, &swapChain, &device);
	if (FAILED(hr))
	{
		printf("D3D10CreateDeviceAndSwapChain失败，错误代码：%u\n", GetLastError());
		DestroyWindow(hwnd);
		UnregisterClass(wc.lpszClassName, wc.hInstance);
		return FALSE;
	}

	//void** pSwapChainVtbl = *reinterpret_cast<void***>(swapChain);
	void** pSwapChainVtbl = *(void***)(swapChain);
	TruePresent = (PF_Present)pSwapChainVtbl[8];

	swapChain->Release();
	device->Release();
	DestroyWindow(hwnd);
	UnregisterClass(wc.lpszClassName, wc.hInstance);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(LPVOID&)TruePresent, hkPresent);
	LONG error = DetourTransactionCommit();
	if (errno == NO_ERROR) printf("Detour succ\n");

	return TRUE;
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
		if (TruePresent != NULL) DetourDetach(&(LPVOID&)TruePresent, hkPresent);
		DetourDetach(&(LPVOID&)TrueEntryPoint, hkEntryPoint);
		DetourTransactionCommit();
		break;
	}

	return TRUE;
}

