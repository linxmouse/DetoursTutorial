#pragma once

typedef HRESULT(WINAPI* TEndScense)(IDirect3DDevice9* pdevice);

extern HWND gHwnd;
extern UINT gWndWidth;
extern UINT gWndHeight;
extern TEndScense TureEndScense;