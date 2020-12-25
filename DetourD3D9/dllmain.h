#pragma once

typedef HRESULT(WINAPI* TEndScense)(IDirect3DDevice9* pdevice);

extern HWND gHwnd;
extern TEndScense TureEndScense;