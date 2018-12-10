// Loader.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <cassert>
#include <iostream>
#include <windows.h>
#include "utility/utility.h"

using namespace std;

struct MapObj
{
	BOOL Access;
	char Path[MAX_PATH];
};

int main(int argc, char** argv)
{
	SetCurrentDirectory(GetExeDir().c_str());

	if (argc != 3)
	{
		string file;
		SplitPath(argv[0], string(), string(), file);
		cout << "usage: " << file << " <game exe> <xp3 file>\n";
		return 1;
	}

	string exePath(MakeFullPath(argv[1]));
	string exeDir(GetPathDir(exePath));
#ifdef _DEBUG
	string dllPath(GetExeDir() + "BlackSheep_d.dll");
#else
	string dllPath(GetExeDir() + "BlackSheep.dll");
#endif
	string xp3Path(MakeFullPath(argv[2]));
	if (xp3Path.size() >= sizeof(MapObj::Path))
	{
		cout << "xp3 path too long.\n";
		return 1;
	}


    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    char cmdline[256];

    strcpy_s(cmdline, sizeof(cmdline), exePath.c_str());
    BOOL b = CreateProcess(NULL, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, exeDir.c_str(), &si, &pi);
    assert(b);

    
    LPVOID remote = VirtualAllocEx(pi.hProcess, NULL, 4096, MEM_COMMIT, PAGE_READWRITE);
    assert(remote);
    

    SIZE_T W;
    WriteProcessMemory(pi.hProcess, remote, dllPath.c_str(), dllPath.size() + 1, &W);
    assert(W == dllPath.size() + 1);


    DWORD tid = 0;
    HANDLE rThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, remote, CREATE_SUSPENDED, &tid);
	printf("pid: %#x, tid: %#x, dll_tid:%#x\n", pi.dwProcessId, pi.dwThreadId, tid);


	SIZE_T MapSize = 4096;
	HANDLE hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT, 0, MapSize, "BlackSheep{13E025E2-B7AA-4141-9B4E-402CCB3C4F33}");
	LPVOID Addr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, MapSize);
	memset(Addr, 0, MapSize);
	MapObj mo;
	mo.Access = FALSE;
	strcpy_s(mo.Path, sizeof(mo.Path), xp3Path.c_str());
	
	memcpy_s(Addr, MapSize, &mo, sizeof(mo));


    b = ResumeThread(rThread);
    assert(b != -1);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(rThread);

	while (((MapObj*)Addr)->Access != TRUE)
	{
		Sleep(1);
	}
	UnmapViewOfFile(Addr);
	CloseHandle(hMap);
    return 0;
}

