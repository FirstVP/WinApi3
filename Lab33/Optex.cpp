/******************************************************************************
������: Optex.cpp
�����: Copyright (c) 2000, ������� ������ (Jeffrey Richter)
******************************************************************************/
//#include "..\CmnHdr.h" /* ��. ���������� A */
#include "Optex.h"
///////////////////////////////////////////////////////////////////////////////
// 0=����������������� ������, 1=����������������, -1=��� ���� �� ���������
BOOL COptex::sm_fUniprocessorHost = -1;
/////////////////////////////////////////////////////////////////////////////// 
PSTR COptex::ConstructObjectName(PSTR pszResult,
	PCSTR pszPrefix, BOOL fUnicode, PVOID pszName) {
	pszResult[0] = 0;
	if (pszName == NULL)
		return(NULL);
	wsprintfA(pszResult, fUnicode ? "%s%S" : "%s%s", pszPrefix, pszName);
	return(pszResult);
}
///////////////////////////////////////////////////////////////////////////////
void COptex::CommonConstructor(DWORD dwSpinCount,
	BOOL fUnicode, PVOID pszName) {
	if (sm_fUniprocessorHost == -1) {
		// �������������� ������ ������; �������� ���������� �����������
		SYSTEM_INFO sinf;
		GetSystemInfo(&sinf);
		sm_fUniprocessorHost = (sinf.dwNumberOfProcessors == 1);
	}
	m_hevt = m_hfm = NULL;
	m_psi = NULL;
	if (pszName == NULL) { // �������� ��������������� �������
		m_hevt = CreateEventA(NULL, FALSE, FALSE, NULL);
		//chASSERT(m_hevt != NULL);
		m_psi = new SHAREDINFO;
		//chASSERT(m_psi != NULL);
		ZeroMemory(m_psi, sizeof(*m_psi));
	}
	else { // �������� �������������� �������
		   // ������ ���������� ANSI, ����� ��������� �������� � Win9x � Windows 2000
		char szResult[100];
		ConstructObjectName(szResult, "Optex_Event_", fUnicode, pszName);
		m_hevt = CreateEventA(NULL, FALSE, FALSE, szResult);
		//chASSERT(m_hevt != NULL);
		ConstructObjectName(szResult, "Optex_MMF_", fUnicode, pszName);
		m_hfm = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
			PAGE_READWRITE, 0, sizeof(*m_psi), szResult);
		//chASSERT(m_hfm != NULL);
		m_psi = (PSHAREDINFO)MapViewOfFile(m_hfm,
			FILE_MAP_WRITE, 0, 0, 0);
		//chASSERT(m_psi != NULL);
		// ����������: �������� m_lLockCount, m_dwThreadId � m_lRecurseCount
		// ��������� SHAREDINFO ���� ���������������� ������� ���������. � �������,
		// �������� ������������� ������ � ������ ��������� ��� �� ����� ������,
		// ��������� � �������������� �������.
	}
	SetSpinCount(dwSpinCount);
}
///////////////////////////////////////////////////////////////////////////////
COptex::~COptex() {
#ifdef _DEBUG
	if (IsSingleProcessOptex() && (m_psi->m_dwThreadId != 0)) {
		// �������������� ������ ������ ���������,
		// ���� �� ������� �����-������ �����
		DebugBreak();
	}
	if (!IsSingleProcessOptex() &&
		(m_psi->m_dwThreadId == GetCurrentThreadId())) {
		// ������������� ������ ������ ���������, ���� �� ������� ��� �����
		DebugBreak();
	}
#endif
	CloseHandle(m_hevt);
	if (IsSingleProcessOptex()) {
		delete m_psi;
	}
	else {
		UnmapViewOfFile(m_psi);
		CloseHandle(m_hfm);
	}
}
///////////////////////////////////////////////////////////////////////////////
void COptex::SetSpinCount(DWORD dwSpinCount) {
	// ����-���������� �� ���������������� ������� �� �����������
	if (!sm_fUniprocessorHost)
		InterlockedExchangePointer((PVOID*)&m_psi->m_dwSpinCount,
		(PVOID)(DWORD_PTR)dwSpinCount);
}
///////////////////////////////////////////////////////////////////////////////
void COptex::Enter() {
	// "��������", ������� ��������� ������
	if (TryEnter())
		return; // ����������, ���������� ����������
				// ��������� ������ �� �������, ��������� � ��������� ��������
	DWORD dwThreadId = GetCurrentThreadId();
	if (InterlockedIncrement(&m_psi->m_lLockCount) == 1) {
		// ������ �� �����, ����� ���� ����� �������� ��� �����
		m_psi->m_dwThreadId = dwThreadId;
		m_psi->m_lRecurseCount = 1;
	}
	else {
		if (m_psi->m_dwThreadId == dwThreadId) {
			// ���� ������ ����������� ������� ������, ����������� ��� ��� ���
			m_psi->m_lRecurseCount++;
		}
		else {
			// ������ ����������� ������� ������, ����
			WaitForSingleObject(m_hevt, INFINITE);
			// ������ �� �����, ����� ���� ����� �������� ��� �����
			m_psi->m_dwThreadId = dwThreadId;
			m_psi->m_lRecurseCount = 1;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
BOOL COptex::TryEnter() {
	DWORD dwThreadId = GetCurrentThreadId();
	BOOL fThisThreadOwnsTheOptex = FALSE; // �������, ��� ����� ������� ��������
	DWORD dwSpinCount = m_psi->m_dwSpinCount; // ������ ����� ������
	do {
		// ���� ������� ����� ���������� = 0, ������ �� �����,
		// � �� ����� ��������� ���
		fThisThreadOwnsTheOptex = (0 ==
			InterlockedCompareExchange(&m_psi->m_lLockCount, 1, 0));
		if (fThisThreadOwnsTheOptex) {
			// ������ �� �����, ����� ���� ����� �������� ��� �����
			m_psi->m_dwThreadId = dwThreadId;
			m_psi->m_lRecurseCount = 1;
		}
		else {
			if (m_psi->m_dwThreadId == dwThreadId) {
				// ���� ������ ����������� ������� ������, ����������� ��� ��� ���
				InterlockedIncrement(&m_psi->m_lLockCount);
				m_psi->m_lRecurseCount++;
				fThisThreadOwnsTheOptex = TRUE;
			}
		}
	} while (!fThisThreadOwnsTheOptex && (dwSpinCount-- > 0));
	// ���������� ���������� ���������� �� ����,
	// ������� ������ ����� �������� ��� ���
	return(fThisThreadOwnsTheOptex);
}
///////////////////////////////////////////////////////////////////////////////
void COptex::Leave() {
#ifdef _DEBUG
	// �������� ������ ����� ���� ��� �����, ������� �� �������
	if (m_psi->m_dwThreadId != GetCurrentThreadId())
		DebugBreak();
#endif
	// ��������� ������� ����� �������� ������� ������ �������
	if (--m_psi->m_lRecurseCount > 0) {
		// ������ ��� ��� ����������� ���
		InterlockedDecrement(&m_psi->m_lLockCount);
	}
	else {
		// ������ ��� ������ �� �����������
		m_psi->m_dwThreadId = 0;
		if (InterlockedDecrement(&m_psi->m_lLockCount) > 0) {
			// ���� ������ ���� ������ ������,
			// ������� � ����������� �������� ���� �� ���
			SetEvent(m_hevt);
		}
	}
}