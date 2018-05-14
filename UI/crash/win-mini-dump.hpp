#pragma once  
#include <windows.h>  
#include <stdlib.h> 
#include <dbghelp.h>
#include <string>
#include "tblive-sdk.h"

#pragma comment(lib,"DbgHelp.lib")

inline BOOL IsDataSectionNeeded(const WCHAR* pModuleName)
{
	if (pModuleName == 0)
	{
		return FALSE;
	}

	WCHAR szFileName[_MAX_FNAME] = L"";
	_wsplitpath(pModuleName, NULL, NULL, szFileName, NULL);

	if (wcsicmp(szFileName, L"ntdll") == 0)
		return TRUE;

	return FALSE;
}

inline BOOL CALLBACK MiniDumpCallback(PVOID                            pParam,
	const PMINIDUMP_CALLBACK_INPUT   pInput,
	PMINIDUMP_CALLBACK_OUTPUT        pOutput)
{
	if (pInput == 0 || pOutput == 0)
		return FALSE;

	switch (pInput->CallbackType)
	{
	case ModuleCallback:
		if (pOutput->ModuleWriteFlags & ModuleWriteDataSeg)
			if (!IsDataSectionNeeded(pInput->Module.FullPath))
				pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg);
	case IncludeModuleCallback:
	case IncludeThreadCallback:
	case ThreadCallback:
	case ThreadExCallback:
		return TRUE;
	default:;
	}

	return FALSE;
}

inline void CreateMiniDump(PEXCEPTION_POINTERS pep)
{
	std::string file_name;
	file_name += GenerateTimeDateString();
	if (App() && !App()->userId.empty()) {
		file_name += "_";
		file_name += App()->userId;
	}

	file_name += ".dmp";
	std::stringstream file_name_stream;
	file_name_stream << "TBLiveStudio\\crashes\\" << file_name.c_str();
	std::string file_path = GetConfigPathPtr(file_name_stream.str().c_str());

	HANDLE hFile = CreateFileA(file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		MINIDUMP_EXCEPTION_INFORMATION mdei;
		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pep;
		mdei.ClientPointers = NULL;

		MINIDUMP_CALLBACK_INFORMATION mci;
		mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)MiniDumpCallback;
		mci.CallbackParam = 0;

		BOOL bOK = MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), hFile, MiniDumpNormal, (pep != 0) ? &mdei : 0, NULL, &mci);
		if (bOK) {
			blog(LOG_INFO, "Succeeded to create dmp file(%s).", file_name.c_str());
			//FILE *fi_fp_3;
			//fi_fp_3 = fopen("C:\\result_3.txt", "a");
			//fprintf(fi_fp_3, "%s", "this is OK.");
			//fclose(fi_fp_3);
		}
		else {
			blog(LOG_INFO, "Failed to create dmp file(%s).", file_name.c_str());
		}
		CloseHandle(hFile);
		int result = TBLiveOSSHandler::upload_crash_file(file_path.c_str(), 
			std::string("crashes/").append(TBLiveEngine::Instance()->GetVerString()).append("/").append(file_name).c_str());
		//FILE *fo_fp;
		//fo_fp = fopen("C:\\result_2.txt", "a");
		//fprintf(fo_fp, "%s", "this is ok");
		//fclose(fo_fp);
	}
}

LONG __stdcall MyUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionInfo)
{
	CreateMiniDump(pExceptionInfo);

	return EXCEPTION_EXECUTE_HANDLER;
}

// 此函数一旦成功调用，之后对 SetUnhandledExceptionFilter 的调用将无效  
void DisableSetUnhandledExceptionFilter()
{
	void* addr = (void*)GetProcAddress(LoadLibrary(LPCTSTR("kernel32.dll")),
		"SetUnhandledExceptionFilter");

	if (addr)
	{
		unsigned char code[16];
		int size = 0;

		code[size++] = 0x33;
		code[size++] = 0xC0;
		code[size++] = 0xC2;
		code[size++] = 0x04;
		code[size++] = 0x00;

		DWORD dwOldFlag, dwTempFlag;
		VirtualProtect(addr, size, PAGE_READWRITE, &dwOldFlag);
		WriteProcessMemory(GetCurrentProcess(), addr, code, size, NULL);
		VirtualProtect(addr, size, dwOldFlag, &dwTempFlag);
	}
}

void SetupWinCrashHandler()
{
	SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
	DisableSetUnhandledExceptionFilter();
}