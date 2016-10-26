#pragma once
#include "windows.h"
class InterProcessCS {
public:
	InterProcessCS(char* pszName, DWORD dwSpinCount);
	~InterProcessCS();
	void SetSpinCount(DWORD dwSpinCount);
	void Enter();
	BOOL TryEnter();
	void Leave();
private:
	typedef struct {
		DWORD m_dwSpinCount;
		long m_lLockCount;
		DWORD m_dwThreadId;
		long m_lRecurseCount;
	} SHAREDINFO, *PSHAREDINFO;
	HANDLE m_hevt;
	HANDLE m_hfm;
	PSHAREDINFO m_psi;
private:
	char* ConstructObjectName(char* pszResult,
		char* pszPrefix, char* pszName);
};
