#pragma once

typedef struct tagEPINFO
{
	DWORD dwPID;
	HWND hwnd;
}EPINFO;
HWND GetMainHWnd(DWORD pid);