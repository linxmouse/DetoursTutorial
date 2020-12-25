// DetoursInjector.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include <detours/detours.h>
#include <io.h>

BOOL Inject(TCHAR* exePath, char* dllPath)
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

	STARTUPINFO sinfo{ 0 };
	PROCESS_INFORMATION pinfo{ 0 };
	sinfo.cb = sizeof(sinfo);
	return  DetourCreateProcessWithDllEx(NULL, exePath, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL,
		_dir, &sinfo, &pinfo, dllPath, (PDETOUR_CREATE_PROCESS_ROUTINEW)CreateProcess);
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		printf("用法:\nDetoursInjector exe dll\n");
		return 0;
	}

	BOOL NoError = TRUE;
	TCHAR argv1[MAX_PATH]{ 0 };
#ifdef _UNICODE
	size_t len = strlen(argv[1]);
	MultiByteToWideChar(CP_ACP, 0, argv[1], (int)len, argv1, (int)len);

	// 判断Dll文件是否存在
	if (_access(argv[2], 0) == -1)
	{
		printf("Dll文件不存在\n");
		NoError = FALSE;
	}

	if (NoError)
	{
		if (!Inject(argv1, argv[2]))
		{
			printf("Inject失败，错误代码：%u\n", GetLastError());
		}
	}
#else
	// 判断Dll文件是否存在
	if (_taccess(argv[2], 0) == -1)
	{
		printf("Dll文件不存在\n");
		NoError = FALSE;
	}

	if (NoError)
	{
		if (!Inject(argv[1], argv[2]))
		{
			printf("Inject失败，错误代码：%u\n", GetLastError());
		}
	}
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
