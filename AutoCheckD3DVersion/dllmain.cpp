// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

typedef IDirect3D9* (WINAPI* TDirect3DCreate9)(UINT SDKVersion);
typedef HRESULT (WINAPI* TD3D11CreateDeviceAndSwapChain)(
    __in_opt IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    __in_ecount_opt(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    __in_opt CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
    __out_opt IDXGISwapChain** ppSwapChain,
    __out_opt ID3D11Device** ppDevice,
    __out_opt D3D_FEATURE_LEVEL* pFeatureLevel,
    __out_opt ID3D11DeviceContext** ppImmediateContext);

static int (WINAPI* TrueEntryPoint)() = NULL;
static int (WINAPI* RawEntryPoint)() = NULL;

TDirect3DCreate9 TrueDirect3DCreate9 = NULL;
TD3D11CreateDeviceAndSwapChain TrueD3D11CreateDeviceAndSwapChain = NULL;
PFN_D3D11_CREATE_DEVICE TrueD3D11CreateDevice = NULL;

IDirect3D9* WINAPI hkDirect3DCreate9(UINT SDKVersion)
{
    printf("Direct3DCreate9 Detected\n");

    return TrueDirect3DCreate9(SDKVersion);
}

HRESULT WINAPI hkD3D11CreateDevice(
    __in_opt IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    __in_ecount_opt(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    __out_opt ID3D11Device** ppDevice,
    __out_opt D3D_FEATURE_LEVEL* pFeatureLevel,
    __out_opt ID3D11DeviceContext** ppImmediateContext)
{
    printf("D3D11CreateDevice Detected\n");
    return TrueD3D11CreateDevice(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels,
        SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
}

HRESULT WINAPI hkD3D11CreateDeviceAndSwapChain(
    __in_opt IDXGIAdapter* pAdapter,
    D3D_DRIVER_TYPE DriverType,
    HMODULE Software,
    UINT Flags,
    __in_ecount_opt(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    __in_opt CONST DXGI_SWAP_CHAIN_DESC* pSwapChainDesc,
    __out_opt IDXGISwapChain** ppSwapChain,
    __out_opt ID3D11Device** ppDevice,
    __out_opt D3D_FEATURE_LEVEL* pFeatureLevel,
    __out_opt ID3D11DeviceContext** ppImmediateContext)
{
    printf("D3D11CreateDeviceAndSwapChain Detected\n");

    return TrueD3D11CreateDeviceAndSwapChain(pAdapter, DriverType, Software, Flags, pFeatureLevels,
        FeatureLevels, SDKVersion,
        pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);
}

int WINAPI hkEntryPoint()
{
    int ret = TrueEntryPoint();
    return ret;
}

void DebugConsole()
{
#ifdef _DEBUG
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONIN$", "r", stdin);
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
#endif // _DEBUG
}

DWORD WINAPI Main(LPVOID arg)
{
    printf("Enter Main\n");

    HMODULE dx9 = NULL;
    HMODULE dx10 = NULL;
    HMODULE dx10_1 = NULL;
    HMODULE dx11 = NULL;
    HMODULE dx11_1 = NULL;

    int Retry = 0;
    while (!dx9 && !dx10 && !dx10_1 && !dx11 && !dx11_1)
    {
        if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, _T("d3d9.dll"), &dx9))
        {
            //printf("Get Module d3d9失败，错误代码：%u\n", GetLastError());
        }
        if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, _T("d3d10.dll"), &dx10))
        {
            //printf("Get Module d3d10失败，错误代码：%u\n", GetLastError());
        }
        if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, _T("d3d10_1.dll"), &dx10_1))
        {
            //printf("Get Module d3d10.1失败，错误代码：%u\n", GetLastError());
        }
        if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, _T("d3d11.dll"), &dx11))
        {
            //printf("Get Module d3d11失败，错误代码：%u\n", GetLastError());
        }
        if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, _T("d3d11_1.dll"), &dx11_1))
        {
            //printf("Get Module d3d11.1失败，错误代码：%u\n", GetLastError());
        }

        if (dx9 != NULL)
        {
            printf("Dirext 9 Detected\n");
        }

        if (dx10 != NULL)
        {
            printf("Dirext 10 Detected\n");
        }

        if (dx10_1 != NULL)
        {
            printf("Dirext 10.1 Detected\n");
        }
        
        if (dx11 != NULL)
        {
            printf("Dirext 11 Detected\n");
        }
        
        if (dx11_1 != NULL)
        {
            printf("Dirext 11.1 Detected\n");
        }

        Sleep(100);
        Retry++;
        if (Retry * 100 > 2000)
        {
            printf("Unsupported Direct3D version, or Direct3D DLL not loaded within 5 seconds.\n");
            break;
        }
    }

    printf("End Main\n");
    return TRUE;
}

BOOL APIENTRY DllMain( HMODULE hModule,
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

        DebugConsole();

        TrueEntryPoint = (int (WINAPI*)())DetourGetEntryPoint(NULL);
        RawEntryPoint = TrueEntryPoint;

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(LPVOID&)TrueEntryPoint, hkEntryPoint);
        DetourTransactionCommit();

        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Main, hModule, 0, NULL);
        break;
    case DLL_PROCESS_DETACH:
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(LPVOID&)TrueEntryPoint, hkEntryPoint);
        DetourTransactionCommit();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

