// SimpleInjector.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <tchar.h>
#include <TlHelp32.h>
#ifndef _UNICODE
#include <io.h>
#endif // !_UNICODE


DWORD GetProcessId(TCHAR* exeName)
{
	PROCESSENTRY32 procEntry;
	procEntry.dwSize = sizeof(procEntry);
	HANDLE hTool32 = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	DWORD ret = 0;
	do
	{
		if (_tcsicmp(exeName, procEntry.szExeFile) == 0)
		{
			ret = procEntry.th32ProcessID;
			break;
		}
	} while (Process32Next(hTool32, &procEntry));

	CloseHandle(hTool32);

	return ret;
}

BOOL UseCreate(TCHAR* exePath, TCHAR* exeDir, TCHAR* dllPath)
{
	STARTUPINFO sinfo = { 0 };
	PROCESS_INFORMATION pinfo = { 0 };
	if (!CreateProcess(NULL, exePath, NULL, NULL, TRUE,
		CREATE_SUSPENDED, NULL, exeDir, &sinfo, &pinfo))
	{
		printf("创建进程失败，错误代码: %u\n", GetLastError());
		return FALSE;
	}

	void* location = VirtualAllocEx(pinfo.hProcess, NULL, MAX_PATH,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (location == NULL)
	{
		printf("申请内存失败，错误代码：%u\n", GetLastError());
		return FALSE;
	}

	// nSize: The number of bytes to be written to the specified process.
	if (!WriteProcessMemory(pinfo.hProcess, location, dllPath,
		(_tcslen(dllPath) + 1) * sizeof(TCHAR),
		NULL))
	{
		printf("写入内存失败，错误代码：%u\n", GetLastError());
		return FALSE;
	}

	HANDLE hThread = CreateRemoteThread(pinfo.hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)LoadLibrary, location, 0, NULL);
	if (hThread == NULL)
	{
		printf("加载Dll失败，错误代码：%u\n", GetLastError());
		return FALSE;
	}

	WaitForSingleObject(hThread, INFINITE);
	VirtualFreeEx(pinfo.hProcess, location, 0, MEM_RELEASE);

	DWORD module;
	// module = STILL_ACTIVE 表示线程正在运行
	// 若线程己经结束, 则module中存储Dll的HMODULE
	GetExitCodeThread(hThread, &module);
	if (module == NULL)
	{
		printf("加载的Dll未执行或加载Dll失败\n");
		TerminateProcess(pinfo.hProcess, 0);
		return FALSE;
	}

	// 恢复
	ResumeThread(pinfo.hThread);
	CloseHandle(hThread);

	return TRUE;
}

BOOL UseOpen(DWORD pid, TCHAR* dllPath)
{
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess == NULL)
	{
		printf("打开进程失败，错误代码: %u\n", GetLastError());
		return FALSE;
	}

	void* location = VirtualAllocEx(hProcess, NULL, MAX_PATH,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (location == NULL)
	{
		printf("申请内存失败，错误代码：%u\n", GetLastError());
		return 0;
	}

	// nSize: The number of bytes to be written to the specified process.
	if (!WriteProcessMemory(hProcess, location, dllPath,
		(_tcslen(dllPath) + 1) * sizeof(TCHAR),
		NULL))
	{
		printf("写入内存失败，错误代码：%u\n", GetLastError());
		return 0;
	}

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)LoadLibrary, location, 0, NULL);
	if (hThread == NULL)
	{
		printf("加载Dll失败，错误代码：%u\n", GetLastError());
		return NULL;
	}

	WaitForSingleObject(hThread, INFINITE);
	VirtualFreeEx(hProcess, location, 0, MEM_RELEASE);

	DWORD module;
	// module = STILL_ACTIVE 表示线程正在运行
	// 若线程己经结束, 则module中存储Dll的HMODULE
	GetExitCodeThread(hThread, &module);
	if (module == NULL)
	{
		printf("加载的Dll未执行或加载Dll失败\n");
		return FALSE;
	}

	CloseHandle(hThread);
	CloseHandle(hProcess);

	return TRUE;
}

BOOL Inject(TCHAR* exePath, TCHAR* dllPath)
{
	TCHAR dir[_MAX_DIR]{ 0 };
	TCHAR drive[_MAX_DRIVE]{ 0 };
	TCHAR fname[_MAX_FNAME]{ 0 };
	TCHAR ext[_MAX_EXT]{ 0 };
	_tsplitpath_s(exePath, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);

	TCHAR _dir[MAX_PATH]{ 0 };
	TCHAR _proc[MAX_PATH]{ 0 };
	// 组合目录名
	_tcscat_s(_dir, drive);
	_tcscat_s(_dir, dir);
	// 组合进程名
	_tcscat_s(_proc, fname);
	_tcscat_s(_proc, ext);

	// 给出的进程路径只有文件名和扩展名
	if (drive == NULL && dir == NULL)
	{
		DWORD pid = GetProcessId(_proc);
		if (pid == NULL)
		{
			printf("获取进程ID失败\n");
			return FALSE;
		}

		return UseOpen(pid, dllPath);
	}
	// 目标进程已经打开
	// 相对路径：drive为空dir不为空
	// 绝对路径：drive不为空dir不为空(drive不为空的情况下dir绝对不为空)
	else
	{
		DWORD pid = GetProcessId(_proc);
		if (pid != NULL)
		{
			return UseOpen(pid, dllPath);
		}
		else
		{
			return UseCreate(exePath, _dir, dllPath);
		}
	}
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		printf("用法:\nSimpleInjector exe dll\n");
		return 0;
	}

	BOOL NoError = TRUE;
	TCHAR argv1[MAX_PATH]{ 0 };
	TCHAR argv2[MAX_PATH]{ 0 };
#ifdef _UNICODE
	size_t len = strlen(argv[1]);
	MultiByteToWideChar(CP_ACP, 0, argv[1], (int)len, argv1, (int)len);
	len = strlen(argv[2]);
	MultiByteToWideChar(CP_ACP, 0, argv[2], (int)len, argv2, (int)len);

	// 判断Dll文件是否存在
	if (_taccess(argv2, 0) == -1)
	{
		printf("Dll文件不存在\n");
		NoError = FALSE;
	}

	if (NoError)
		Inject(argv1, argv2);
#else
	// 判断Dll文件是否存在
	if (_taccess(argv[2], 0) == -1)
	{
		printf("Dll文件不存在\n");
		NoError = FALSE;
	}

	if (NoError)
		Inject(argv[1], argv[2]);
#endif // _UNICODE

	printf("Press Escape To Exit...\n");
	while (true)
	{
		if (GetAsyncKeyState(VK_ESCAPE))
		{
			break;
		}
	}
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
