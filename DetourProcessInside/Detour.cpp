#include "Detour.h"
#include <Windows.h>
#include <detours/detours.h>

Detour::Detour(void* src, void* dest):
	targetPtr(src), detourFunc(dest)
{
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&targetPtr, detourFunc);
	DetourTransactionCommit();
}

Detour::~Detour()
{
	DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&targetPtr, detourFunc);
    DetourTransactionCommit();
}
