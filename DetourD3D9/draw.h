#pragma once

extern WNDPROC OWndProc;

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
HRESULT WINAPI hkEndScense(IDirect3DDevice9* pdevice);