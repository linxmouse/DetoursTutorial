#include "pch.h"
#include "wnd.h"

static BOOL CALLBACK cbEnumProc(HWND hwnd, LPARAM lparam)
{
	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);
	EPINFO* pwi = (EPINFO*)lparam;
	if (pid == pwi->dwPID)
	{
		HWND _hwnd = GetParent(hwnd);
		if (_hwnd != NULL)
		{
			pwi->hwnd = _hwnd;
			return FALSE;
		}
	}

	return TRUE;
}

// 获取主窗口句柄
HWND GetMainHWnd(DWORD pid)
{
	EPINFO wi{ 0 };
	wi.dwPID = pid;
	EnumWindows(cbEnumProc, (LPARAM)&wi);

	return wi.hwnd;
}