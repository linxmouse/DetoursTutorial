// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

static int (WINAPI* TrueEntryPoint)() = NULL;
static int (WINAPI* RawEntryPoint)() = NULL;

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
    WaitForSingleObject(arg, INFINITE);

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

