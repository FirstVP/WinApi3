#pragma once
#include "windows.h"
class InterProcessCS {
public:
	InterProcessCS(PCWSTR pszName, DWORD dwSpinCount);
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
	PSTR ConstructObjectName(PSTR pszResult,
		PCSTR pszPrefix, PCWSTR pszName);
};
