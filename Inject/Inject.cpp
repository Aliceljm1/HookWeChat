#include <iostream>
#include <windows.h>
#include <atlstr.h> //CString的头文件
#include <Tlhelp32.h>

LPCTSTR exeName = _T("WeChat.exe");
LPCTSTR dllName = _T("hookDll.dll");

// 获取PID
UINT GetProcessIdByName(LPCTSTR pszExeFile)
{
	UINT nProcessID = 0;
	PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);

	if (hSnapshot != INVALID_HANDLE_VALUE)
	{
		if (Process32First(hSnapshot, &pe))
		{
			while (Process32Next(hSnapshot, &pe))
			{
				if (lstrcmpi(pszExeFile, pe.szExeFile) == 0)
				{
					nProcessID = pe.th32ProcessID;
					break;
				}
			}
		}
		CloseHandle(hSnapshot);
	}
	return nProcessID;
}

// 提升进程访问权限
bool enableDebugPriv()
{
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)
		)
	{
		return false;
	}
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue))
	{
		CloseHandle(hToken);
		return false;
	}
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL))
	{
		CloseHandle(hToken);
		return false;
	}
	return true;
}

// 卸载DLL
bool UnInjectDll(const TCHAR* ptszDllFile, DWORD dwProcessId)
{
	if (dwProcessId == 0)
	{
		MessageBox(NULL,
			L"The target process have not been found !",
			L"Notice",
			MB_ICONINFORMATION | MB_OK
		);
		return FALSE;
	}

	// 参数无效   
	if (NULL == ptszDllFile || 0 == ::_tcslen(ptszDllFile))
	{
		return false;
	}
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	HANDLE hProcess = NULL;
	HANDLE hThread = NULL;
	// 获取模块快照   
	hModuleSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessId);
	if (INVALID_HANDLE_VALUE == hModuleSnap)
	{
		return false;
	}
	MODULEENTRY32 me32;
	memset(&me32, 0, sizeof(MODULEENTRY32));
	me32.dwSize = sizeof(MODULEENTRY32);
	// 开始遍历   
	if (FALSE == ::Module32First(hModuleSnap, &me32))
	{
		::CloseHandle(hModuleSnap);
		return false;
	}
	// 遍历查找指定模块   
	bool isFound = false;
	do
	{
		isFound = (0 == ::_tcsicmp(me32.szModule, ptszDllFile) || 0 == ::_tcsicmp(me32.szExePath, ptszDllFile));
		if (isFound) // 找到指定模块   
		{
			break;
		}
	} while (TRUE == ::Module32Next(hModuleSnap, &me32));
	::CloseHandle(hModuleSnap);
	if (false == isFound)
	{
		std::cout << "找不到DLL" << std::endl;
		return false;
	}
	// 获取目标进程句柄   
	hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
	if (NULL == hProcess)
	{
		return false;
	}
	// 从 Kernel32.dll 中获取 FreeLibrary 函数地址   
	LPTHREAD_START_ROUTINE lpThreadFun = (PTHREAD_START_ROUTINE)::GetProcAddress(::GetModuleHandle(_T("Kernel32")), "FreeLibrary");
	if (NULL == lpThreadFun)
	{
		::CloseHandle(hProcess);
		return false;
	}
	// 创建远程线程调用 FreeLibrary   
	hThread = ::CreateRemoteThread(hProcess, NULL, 0, lpThreadFun, me32.modBaseAddr /* 模块地址 */, 0, NULL);
	if (NULL == hThread)
	{
		::CloseHandle(hProcess);
		return false;
	}
	// 等待远程线程结束   
	::WaitForSingleObject(hThread, INFINITE);
	// 清理   
	::CloseHandle(hThread);
	::CloseHandle(hProcess);
	std::cout << "卸载成功" << std::endl;
	return true;
}

char* strToChar(CString str)
{
	char* ptr;
#ifdef _UNICODE
	LONG len;
	len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	ptr = new char[len + 1];
	memset(ptr, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, str, -1, ptr, len + 1, NULL, NULL);
#else
	ptr = new char[str.GetAllocLength() + 1];
	sprintf(ptr, _T("%s"), str);
#endif
	return ptr;
}

// 注入DLL
bool InjectDll(DWORD pid)
{
	HANDLE hprocess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	if (!hprocess)
	{
		std::cout << "can not get handle" << std::endl;
		return 1;
	}
	CString str;
	GetModuleFileName(NULL, str.GetBufferSetLength(MAX_PATH + 1), MAX_PATH + 1);
	int last = str.ReverseFind('\\');
	str = str.Left(last); //DLLPath为exe文件的绝对路径
	str.Format(L"%s%s", str, L"\\hookDll.dll");

	// DLL的路径，和exe在同一目录下
	const char* DLLPath = strToChar(str);
	SIZE_T PathSize = (strlen(DLLPath) + 1) * sizeof(TCHAR);

	LPVOID StartAddress = VirtualAllocEx(hprocess, NULL, PathSize, MEM_COMMIT, PAGE_READWRITE);
	if (!StartAddress)
	{
		std::cout << "开辟内存失败" << std::endl;
		return 1;
	}
	if (!WriteProcessMemory(hprocess, StartAddress, DLLPath, PathSize, NULL))
	{
		std::cout << "无法写入DLL路径" << std::endl;
		return 1;
	}
	PTHREAD_START_ROUTINE pfnStartAddress = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
	if (!pfnStartAddress)
	{
		std::cout << "无法获取函数地址" << std::endl;
		return 1;
	}
	HANDLE hThread = CreateRemoteThreadEx(hprocess, NULL, NULL, pfnStartAddress, StartAddress, NULL, NULL, NULL);
	if (!hThread)
	{
		std::cout << "创建线程失败" << std::endl;
		return 1;
	}
	//WaitForSingleObject(hThread, INFINITE);//等待DLL结束
	std::cout << "注入成功" << std::endl;
	CloseHandle(hThread);
	CloseHandle(hprocess);
}

int main(int argc, char* argv[])
{
	// 提高访问权限
	enableDebugPriv();
	// 获取微信的进程ID
	DWORD pid = GetProcessIdByName(exeName);
	// 注入
	InjectDll(pid);
	Sleep(100);
	// 卸载DLL
	UnInjectDll(dllName, pid);

	return 0;
}
