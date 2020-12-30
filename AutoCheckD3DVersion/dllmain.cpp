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

HANDLE OpenAndConnectPipe(TCHAR* pipe)
{
	HANDLE ret = NULL;
	TCHAR buffer[MAX_PATH]{ 0 };
	if (!WaitNamedPipe(pipe, 3000))
	{
		_stprintf_s(buffer, _T("管道忙或没有启动，错误代码：%u\n"), GetLastError());
		OutputDebugString(buffer);

		return ret;
	}

	ret = CreateFile(pipe,
		GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (ret == INVALID_HANDLE_VALUE)
	{
		ZeroMemory(buffer, MAX_PATH);
		_stprintf_s(buffer, _T("管道打开失败，错误代码：%u\n"), GetLastError());
		OutputDebugString(buffer);

		return ret;
	}

	return ret;
}

void ClosePipe(HANDLE hPipe)
{
	if (hPipe != NULL)
	{
		FlushFileBuffers(hPipe);
		DisconnectNamedPipe(hPipe);
		CloseHandle(hPipe);
		hPipe = NULL;
	}
}

DWORD WINAPI Main(LPVOID arg)
{
	HANDLE hPipe = OpenAndConnectPipe((TCHAR*)_T("\\\\.\\pipe\\ChkDxVer"));

	HMODULE dx9 = NULL;
	HMODULE dx10 = NULL;
	HMODULE dx10_1 = NULL;
	HMODULE dx11 = NULL;
	HMODULE dx11_1 = NULL;

	int Retry = 0;
	BOOL error = FALSE;
	char msg[512]{ 0 };
	while (!dx9 && !dx10 && !dx10_1 && !dx11 && !dx11_1)
	{
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, _T("d3d9.dll"), &dx9);
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, _T("d3d10.dll"), &dx10);
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, _T("d3d10_1.dll"), &dx10_1);
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, _T("d3d11.dll"), &dx11);
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, _T("d3d11_1.dll"), &dx11_1);

		if (dx9 != NULL)
		{
			printf("Dirext 9 Detected\n");
			if (strlen(msg) == 0) strcat_s(msg, "Dx9");
			else strcat_s(msg, ", Dx9");
		}
		if (dx10 != NULL)
		{
			printf("Dirext 10 Detected\n");
			if (strlen(msg) == 0) strcat_s(msg, "Dx10");
			else strcat_s(msg, ", Dx10");
		}
		if (dx10_1 != NULL)
		{
			printf("Dirext 10.1 Detected\n");
			if (strlen(msg) == 0) strcat_s(msg, "Dx10.1");
			else strcat_s(msg, ", Dx10.1");
		}
		if (dx11 != NULL)
		{
			printf("Dirext 11 Detected\n");
			if (strlen(msg) == 0) strcat_s(msg, "Dx11");
			else strcat_s(msg, ", Dx11");
		}
		if (dx11_1 != NULL)
		{
			printf("Dirext 11.1 Detected\n");
			if (strlen(msg) == 0) strcat_s(msg, "Dx11.1");
			else strcat_s(msg, ", Dx11.1");
		}

		Sleep(500);
		Retry++;
		if (Retry * 500 > 5000)
		{
			error = TRUE;
			printf("Unsupported Direct3D version, or Direct3D DLL not loaded within 5 seconds.\n");
			break;
		}
	}

	if (!error) WriteFile(hPipe, msg, strlen(msg), NULL, NULL);

	ClosePipe(hPipe);
	if (!TerminateProcess(GetCurrentProcess(), 0))
	{
		MessageBox(NULL, _T("TerminateProcess失败"), _T(""), MB_OK);
	}

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
	}
	return TRUE;
}

