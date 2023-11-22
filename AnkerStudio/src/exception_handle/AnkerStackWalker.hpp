#include <wx/stackwalk.h>
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <DbgHelp.h>
#include <string>
#include <boost/log/trivial.hpp>
#include "libslic3r/Utils.hpp"

using namespace std;

#pragma comment(lib, "DbgHelp.lib")

const int MAX_ADDRESS_LENGTH = 32;
const int MAX_NAME_LENGTH = 1024;
struct CrashInfo
{
	CHAR ErrorCode[MAX_ADDRESS_LENGTH];
	CHAR Address[MAX_ADDRESS_LENGTH];
	CHAR Flags[MAX_ADDRESS_LENGTH];
};

struct CallStackInfo
{
	CHAR ModuleName[MAX_NAME_LENGTH];
	CHAR MethodName[MAX_NAME_LENGTH];
	CHAR FileName[MAX_NAME_LENGTH];
	CHAR LineNumber[MAX_NAME_LENGTH];
};


class AnkerStackWalker : public wxStackWalker
{
public:
	AnkerStackWalker() : wxStackWalker() { }

	virtual void WalkFromException() {
		wxStackWalker::WalkFromException();
	}

	virtual void OnStackFrame(const wxStackFrame& frame) {
		// print stack info to log
		ANKER_LOG_INFO << "File:" << frame.GetFileName() << ", line:" << frame.GetLine() << ", function :" << frame.GetName() << std::endl;
	}
};


void SafeStrCpy(char* szDest, size_t nMaxDestSize, const char* szSrc)
{
	if (nMaxDestSize <= 0) return;
	if (strlen(szSrc) < nMaxDestSize)
	{
		strcpy_s(szDest, nMaxDestSize, szSrc);
	}
	else
	{
		strncpy_s(szDest, nMaxDestSize, szSrc, nMaxDestSize);
		szDest[nMaxDestSize - 1] = '\0';
	}
}


CrashInfo GetCrashInfo(const EXCEPTION_RECORD* pRecord)
{
	CrashInfo crashinfo;
	SafeStrCpy(crashinfo.Address, MAX_ADDRESS_LENGTH, "N/A");
	SafeStrCpy(crashinfo.ErrorCode, MAX_ADDRESS_LENGTH, "N/A");
	SafeStrCpy(crashinfo.Flags, MAX_ADDRESS_LENGTH, "N/A");

	sprintf_s(crashinfo.Address, "%08X", pRecord->ExceptionAddress);
	sprintf_s(crashinfo.ErrorCode, "%08X", pRecord->ExceptionCode);
	sprintf_s(crashinfo.Flags, "%08X", pRecord->ExceptionFlags);

	return crashinfo;
}

vector<CallStackInfo> GetCallStack(const CONTEXT* pContext)
{
	HANDLE hProcess = GetCurrentProcess();

	SymInitialize(hProcess, NULL, TRUE);

	vector<CallStackInfo> arrCallStackInfo;

	CONTEXT c = *pContext;

	STACKFRAME64 sf;
	memset(&sf, 0, sizeof(STACKFRAME64));
	DWORD dwImageType = IMAGE_FILE_MACHINE_I386;

	//
#ifdef _M_IX86
	sf.AddrPC.Offset = c.Eip;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Offset = c.Esp;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Offset = c.Ebp;
	sf.AddrFrame.Mode = AddrModeFlat;
#elif _M_X64
	dwImageType = IMAGE_FILE_MACHINE_AMD64;
	sf.AddrPC.Offset = c.Rip;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrFrame.Offset = c.Rsp;
	sf.AddrFrame.Mode = AddrModeFlat;
	sf.AddrStack.Offset = c.Rsp;
	sf.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
	dwImageType = IMAGE_FILE_MACHINE_IA64;
	sf.AddrPC.Offset = c.StIIP;
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrFrame.Offset = c.IntSp;
	sf.AddrFrame.Mode = AddrModeFlat;
	sf.AddrBStore.Offset = c.RsBSP;
	sf.AddrBStore.Mode = AddrModeFlat;
	sf.AddrStack.Offset = c.IntSp;
	sf.AddrStack.Mode = AddrModeFlat;
#else
#error "Platform not supported!"
#endif

	HANDLE hThread = GetCurrentThread();

	while (true)
	{
		if (!StackWalk64(dwImageType, hProcess, hThread, &sf, &c, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
		{
			break;
		}

		if (sf.AddrFrame.Offset == 0)
		{
			break;
		}

		CallStackInfo callstackinfo;
		SafeStrCpy(callstackinfo.MethodName, MAX_NAME_LENGTH, "N/A");
		SafeStrCpy(callstackinfo.FileName, MAX_NAME_LENGTH, "N/A");
		SafeStrCpy(callstackinfo.ModuleName, MAX_NAME_LENGTH, "N/A");
		SafeStrCpy(callstackinfo.LineNumber, MAX_NAME_LENGTH, "N/A");

		BYTE symbolBuffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_NAME_LENGTH];
		IMAGEHLP_SYMBOL64* pSymbol = (IMAGEHLP_SYMBOL64*)symbolBuffer;
		memset(pSymbol, 0, sizeof(IMAGEHLP_SYMBOL64) + MAX_NAME_LENGTH);

		pSymbol->SizeOfStruct = sizeof(symbolBuffer);
		pSymbol->MaxNameLength = MAX_NAME_LENGTH;

		DWORD symDisplacement = 0;

		if (SymGetSymFromAddr64(hProcess, sf.AddrPC.Offset, NULL, pSymbol))
		{
			SafeStrCpy(callstackinfo.MethodName, MAX_NAME_LENGTH, pSymbol->Name);
		}

		IMAGEHLP_LINE64 lineInfo;
		memset(&lineInfo, 0, sizeof(IMAGEHLP_LINE64));

		lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

		DWORD dwLineDisplacement;

		if (SymGetLineFromAddr64(hProcess, sf.AddrPC.Offset, &dwLineDisplacement, &lineInfo))
		{
			SafeStrCpy(callstackinfo.FileName, MAX_NAME_LENGTH, lineInfo.FileName);
			sprintf_s(callstackinfo.LineNumber, "%d", lineInfo.LineNumber);
		}

		IMAGEHLP_MODULE64 moduleInfo;
		memset(&moduleInfo, 0, sizeof(IMAGEHLP_MODULE64));

		moduleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);

		if (SymGetModuleInfo64(hProcess, sf.AddrPC.Offset, &moduleInfo))
		{
			SafeStrCpy(callstackinfo.ModuleName, MAX_NAME_LENGTH, moduleInfo.ModuleName);
		}

		arrCallStackInfo.push_back(callstackinfo);
	}

	SymCleanup(hProcess);

	return arrCallStackInfo;
}

LONG ApplicationCrashHandler(EXCEPTION_POINTERS* pException)
{
#ifdef _M_IX86
	if (pException->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
	{
		static char TempStack[1024 * 128];
		__asm mov eax, offset TempStack[1024 * 128];
		__asm mov esp, eax;
	}
#endif

	CrashInfo crashinfo = GetCrashInfo(pException->ExceptionRecord);
	ANKER_LOG_ERROR << ("Application is crashed.  errCode:" + string(crashinfo.ErrorCode) + " errAddr:" + crashinfo.Address + " errFlag:" + crashinfo.Flags);
	vector<CallStackInfo> arrCallStackInfo = GetCallStack(pException->ContextRecord);

	for (vector<CallStackInfo>::iterator i = arrCallStackInfo.begin(); i != arrCallStackInfo.end(); ++i)
	{
		CallStackInfo callstackinfo = (*i);
		ANKER_LOG_ERROR << string(callstackinfo.MethodName) + "() : [" + callstackinfo.ModuleName + "] (File: " + callstackinfo.FileName + " @Line " + callstackinfo.LineNumber + ")";
	}
	return EXCEPTION_EXECUTE_HANDLER;
}