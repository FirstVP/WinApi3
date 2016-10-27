#pragma once
#include "windows.h"
class InterProcessCS {
public:
	InterProcessCS(char* pszName, DWORD dwSpinCount);
	~InterProcessCS();
	void SetSpinCount(DWORD dwSpinCount);
	void EnterCriticalSection();
	BOOL TryEnterCriticalSection();
	void LeaveCriticalSection();
private:
	typedef struct {
		DWORD SpinCount;
		long LockCount;
		DWORD OwningThread;
		long RecursionCount;
		HANDLE LockEvent;
	} STATUS;
	HANDLE mappedFile;
	STATUS* status;
private:
	char* GenerateName(char* pszResult,
		char* pszPrefix, char* pszName);
	void TakeByThread(DWORD threadId);
};
