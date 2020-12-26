// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "wnd.h"

typedef HRESULT(STDMETHODCALLTYPE* PF_Present)(
	IDXGISwapChain* pSwapChain, 
	UINT syncInterval, UINT flags);
//typedef HRESULT(STDMETHODCALLTYPE* PF_ResizeBuffers)(
//	IDXGISwapChain* pSwapChain, 
//	UINT BufferCount, 
//	UINT Width, UINT Height, 
//	DXGI_FORMAT NewFormat, 
//	UINT SwapChainFlags);
//typedef HRESULT (STDMETHODCALLTYPE* PF_GetDevice)(REFIID riid, void** ppDevice);
//typedef ULONG(STDMETHODCALLTYPE* PF_Release)(IDXGISwapChain* pSwapChain);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND gHwnd;
static BOOL bInitialized = FALSE;
static IDXGISwapChain* pSwapChain;
static ID3D11Device* pDevice;
static ID3D11DeviceContext* pDeviceContext;

static int (WINAPI* TrueEntryPoint)(VOID) = NULL;
static int (WINAPI* RawEntryPoint)(VOID) = NULL;

//PF_Release TrueRelease = NULL;
//PF_GetDevice TrueGetDevice = NULL;
PF_Present TruePresent = NULL;
//PF_ResizeBuffers TrueResizeBuffers = NULL;

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
			ImGui_ImplWin32_Shutdown();
			ImGui_ImplDX11_Shutdown();
			ImGui::DestroyContext();

			// 释放设备
			if (pDeviceContext) pDeviceContext->Release();
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

//HRESULT STDMETHODCALLTYPE hkGetDevice(REFIID riid, void** ppDevice)
//{
//	return TrueGetDevice(riid, &(*ppDevice));
//}

//ULONG STDMETHODCALLTYPE hkRelease(IDXGISwapChain* pSwapChain)
//{
//	return TrueRelease(pSwapChain);
//}

HRESULT STDMETHODCALLTYPE hkPresent(IDXGISwapChain* lpSwapChain, UINT SyncInterval, UINT Flags)
{
	pSwapChain = lpSwapChain;
	HRESULT hr = lpSwapChain->GetDevice(__uuidof(ID3D11Device), (LPVOID*)&pDevice);
	if (FAILED(hr)) return TruePresent(lpSwapChain, SyncInterval, Flags);
	pDevice->GetImmediateContext(&pDeviceContext);
	
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
		//ImGuiIO& io = ImGui::GetIO();

		ImGui_ImplWin32_Init(gHwnd);
		//ImGui_ImplDX11_Init(pDevice, pDeviceContext);

		bInitialized = TRUE;
	}

	ImGui_ImplDX11_Init(pDevice, pDeviceContext);
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	
	// 释放
	ImGui_ImplDX11_Shutdown();
	pDeviceContext->Release();
	pDeviceContext = NULL;
	pDevice->Release();
	pDevice = NULL;
	if (pSwapChain != lpSwapChain) 
	{ 
		pSwapChain->Release(); 
		pSwapChain = lpSwapChain;
	}

	return TruePresent(lpSwapChain, SyncInterval, Flags);
}

//HRESULT STDMETHODCALLTYPE hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount,
//	UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
//{
//	// to deal with 'the direct3d device has a non-zero reference count'
//	ImGui_ImplDX11_InvalidateDeviceObjects();
//
//	return TrueResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
//}

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
	scd.OutputWindow = hwnd;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.Windowed = TRUE;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	D3D_FEATURE_LEVEL featrueLevelReq = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL featrueLevelSup;
	
	IDXGISwapChain* swapChain;
	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
		0, &featrueLevelReq, 1, D3D11_SDK_VERSION, &scd, &swapChain, &device, &featrueLevelSup, &deviceContext);
	if (FAILED(hr))
	{
		printf("D3D11CreateDeviceAndSwapChain失败，错误代码：%u\n", GetLastError());
		DestroyWindow(hwnd);
		return FALSE;
	}

	// F12查看定义
	//swapChain->GetDevice
	//swapChain->ResizeTarget
	//swapChain->Release

	void** pSwapChainVtbl = *reinterpret_cast<void***>(swapChain);
	//TrueRelease = (PF_Release)pSwapChainVtbl[2];
	//TrueGetDevice = (PF_GetDevice)pSwapChainVtbl[7];
	TruePresent = (PF_Present)pSwapChainVtbl[8];
	//TrueResizeBuffers = (PF_ResizeBuffers)pSwapChainVtbl[13];

	deviceContext->Release();
	swapChain->Release();
	device->Release();
	DestroyWindow(hwnd);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	//DetourAttach(&(LPVOID&)TrueRelease, hkRelease);
	//DetourAttach(&(LPVOID&)TrueGetDevice, hkGetDevice);
	DetourAttach(&(LPVOID&)TruePresent, hkPresent);
	//DetourAttach(&(LPVOID&)TrueResizeBuffers, hkResizeBuffers);
	DetourTransactionCommit();

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
		//if (TrueRelease != NULL) DetourDetach(&(LPVOID&)TrueRelease, hkRelease);
		//if (TrueGetDevice != NULL) DetourDetach(&(LPVOID&)TrueGetDevice, hkGetDevice);
		if (TruePresent != NULL) DetourDetach(&(LPVOID&)TruePresent, hkPresent);
		//if (TrueResizeBuffers != NULL) DetourDetach(&(LPVOID&)TrueResizeBuffers, hkResizeBuffers);
		DetourDetach(&(LPVOID&)TrueEntryPoint, hkEntryPoint);
		DetourTransactionCommit();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

