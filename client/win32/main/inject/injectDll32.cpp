
#include <stdio.h> 
#include "stdafx.h"
#include <windows.h>
#include <io.h>

#include "common.h"
#include "injectDll32.h"
#include "inject.h"

static HANDLE MyCreateRemoteThread(HANDLE hProcess, LPTHREAD_START_ROUTINE pThreadProc, LPVOID pRemoteBuf)
{
    HANDLE      hThread = NULL;
    FARPROC     pFunc = NULL;

    if( is_system_VistaOrLater() )    // Vista, 7, Server2008
    {
        pFunc = GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "NtCreateThreadEx");
        if( pFunc == NULL )
        {
            return NULL;
        }

        ((PFNTCREATETHREADEX)pFunc)(&hThread,
                                    0x1FFFFF,
                                    NULL,
                                    hProcess,
                                    pThreadProc,
                                    pRemoteBuf,
                                    FALSE,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
    }
    else                    // 2000, XP, Server2003
    {
        hThread = CreateRemoteThread(hProcess, 
                                     NULL, 
                                     0, 
                                     pThreadProc, 
                                     pRemoteBuf, 
                                     0, 
                                     NULL);
    }

    return hThread;
}

int inject_dll_32(const char *proc_name, uint64_t proc_id, HANDLE hProcess, char *dll_name)
{
	SIZE_T dwSize = strlen(dll_name) + 1;
	SIZE_T dwHasWrite;
	LPVOID lpRemoteBuf = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (WriteProcessMemory(hProcess, lpRemoteBuf, dll_name, dwSize, &dwHasWrite))
	{
		if (dwHasWrite != dwSize)
		{
			_LOG_ERROR(_T("WriteProcessMemory %s failed: %d, ret %d"), proc_name, GetLastError(), dwHasWrite);

			VirtualFreeEx(hProcess, lpRemoteBuf, dwSize, MEM_COMMIT);
			return -1;
		}
	}
	else
	{
		_LOG_ERROR(_T("WriteProcessMemory %s failed: %d"), proc_name, GetLastError());

		VirtualFreeEx(hProcess, lpRemoteBuf, dwSize, MEM_COMMIT);
		return -1;
	}
	
	//预编译，支持Unicode
#ifdef _UNICODE
	PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)::GetProcAddress(::GetModuleHandle(_T("Kernel32.dll")), "LoadLibraryW");
#else
	PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)::GetProcAddress(::GetModuleHandle(_T("Kernel32.dll")), "LoadLibraryA");
#endif

	HANDLE hNewRemoteThread = MyCreateRemoteThread(hProcess, pfnThreadRtn, lpRemoteBuf);
	if (hNewRemoteThread == NULL)
	{
		_LOG_ERROR(_T("CreateRemoteThread %s failed: %d"), proc_name, GetLastError());

		VirtualFreeEx(hProcess, lpRemoteBuf, dwSize, MEM_COMMIT);
		return -1;
	}

	// 等待远程线程结束  
	::WaitForSingleObject(hNewRemoteThread, INFINITE);
	DWORD ExitCode = 0;
	::GetExitCodeThread(hNewRemoteThread, &ExitCode);
	_LOG_INFO(_T("LoadLibrary return %d"), ExitCode);
	
	::VirtualFreeEx(hProcess, lpRemoteBuf, dwSize, MEM_DECOMMIT);
	::CloseHandle(hNewRemoteThread);

	_LOG_INFO(_T("load success: %s:%d"), proc_name,proc_id);
	return 0;
}

int unInject_dll_32(const char *proc_name, uint64_t proc_id, HANDLE hProcess, char *dll_name)
{
	SIZE_T dwSize = strlen(dll_name) + 1;
	SIZE_T dwHasWrite;
	LPVOID lpRemoteBuf = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (WriteProcessMemory(hProcess, lpRemoteBuf, dll_name, dwSize, &dwHasWrite))
	{
		if (dwHasWrite != dwSize)
		{
			_LOG_ERROR(_T("WriteProcessMemory %s failed: %d, ret %d"), proc_name, GetLastError(), dwHasWrite);

			VirtualFreeEx(hProcess, lpRemoteBuf, dwSize, MEM_COMMIT);
			return -1;
		}
	}
	else
	{
		_LOG_ERROR(_T("WriteProcessMemory %s failed: %d"), proc_name, GetLastError());

		VirtualFreeEx(hProcess, lpRemoteBuf, dwSize, MEM_COMMIT);
		return -1;
	}

	HANDLE hNewRemoteThread = CreateRemoteThread(hProcess, 
											NULL, 
											0, 
											(PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"),"GetModuleHandleA"),
											lpRemoteBuf, 
											0, 
											0);
	if (hNewRemoteThread == NULL)
	{
		_LOG_ERROR(_T("CreateRemoteThread %s failed: %d"), proc_name, GetLastError());

		VirtualFreeEx(hProcess, lpRemoteBuf, dwSize, MEM_COMMIT);
		return -1;
	}

	//等待远程线程结束  
	::WaitForSingleObject(hNewRemoteThread, INFINITE);
	/*获取dll的句柄*/
	HANDLE hDllHandle;
	GetExitCodeThread(hNewRemoteThread, (DWORD*)&hDllHandle);

	if (hDllHandle == 0)
	{
		_LOG_ERROR(_T("remote thread not loaded, %s:%d"), proc_name, GetLastError());

		// 清理  
		::VirtualFreeEx(hProcess, lpRemoteBuf, dwSize, MEM_DECOMMIT);
		::CloseHandle(hNewRemoteThread);
		return -1;
	}

	//把FreeLibrary注入到进程，释放注入的DLL  
    hNewRemoteThread = CreateRemoteThread(hProcess,  
                                   0,  
                                   0,  
                                   (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"),"FreeLibrary"),
                                   hDllHandle,  
                                   0,
                                   0);

    //释放  
    WaitForSingleObject(hNewRemoteThread, INFINITE);

    // 清理  
	::VirtualFreeEx(hProcess, lpRemoteBuf, dwSize, MEM_DECOMMIT);
	::CloseHandle(hNewRemoteThread);

	_LOG_INFO(_T("unload success: %s:%d"), proc_name,proc_id);
    return 0;
}

int is_dll_injected_32(const char *proc_name, uint64_t proc_id, HANDLE hProcess, char *dll_name, bool *isInjected)
{
	SIZE_T dwSize = strlen(dll_name) + 1;
	SIZE_T dwHasWrite;
	LPVOID lpRemoteBuf = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE);
	if (WriteProcessMemory(hProcess, lpRemoteBuf, dll_name, dwSize, &dwHasWrite))
	{
		if (dwHasWrite != dwSize)
		{
			_LOG_ERROR(_T("WriteProcessMemory %s failed: %d, ret %d"), proc_name, GetLastError(), dwHasWrite);
			VirtualFreeEx(hProcess, lpRemoteBuf, dwSize, MEM_COMMIT);
			return -1;
		}
	}
	else
	{
		_LOG_ERROR(_T("WriteProcessMemory %s failed: %d"), proc_name, GetLastError());

		VirtualFreeEx(hProcess, lpRemoteBuf, dwSize, MEM_COMMIT);
		return -1;
	}

	HANDLE hNewRemoteThread = CreateRemoteThread(hProcess, 
											NULL, 
											0, 
											(PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"),"GetModuleHandleA"),
											lpRemoteBuf, 
											0, 
											0);
	if (hNewRemoteThread == NULL)
	{
		_LOG_ERROR(_T("CreateRemoteThread %s failed: %d"), proc_name, GetLastError());

		VirtualFreeEx(hProcess, lpRemoteBuf, dwSize, MEM_COMMIT);
		return -1;
	}

	//等待远程线程结束  
	::WaitForSingleObject(hNewRemoteThread, INFINITE);
	/*获取dll的句柄*/
	HANDLE hDllHandle;
	GetExitCodeThread(hNewRemoteThread, (DWORD*)&hDllHandle);

	if (hDllHandle == 0)
	{
		*isInjected = false;
	}
	else
	{
		*isInjected = true;
	}

	return 0;
}
