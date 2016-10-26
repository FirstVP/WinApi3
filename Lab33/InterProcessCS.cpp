
#include "InterProcessCS.h"
#include "stdio.h"
#define BUFFER_OBJECT_NAME_SIZE 256
PSTR InterProcessCS::ConstructObjectName(PSTR pszResult,
	PCSTR pszPrefix, PCWSTR pszName) {
	pszResult[0] = 0;
	wsprintfA(pszResult, "%s%S", pszPrefix, pszName);
	return(pszResult);
}

InterProcessCS::InterProcessCS(PCWSTR pszName, DWORD dwSpinCount) {
	char szResult[BUFFER_OBJECT_NAME_SIZE];
	ConstructObjectName(szResult, "EVENT", pszName);
	m_hevt = CreateEventA(NULL, FALSE, FALSE, szResult);

	ConstructObjectName(szResult, "FILE", pszName);
	m_hfm = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
		PAGE_READWRITE, 0, sizeof(*m_psi), szResult);

	m_psi = (PSHAREDINFO)MapViewOfFile(m_hfm,
		FILE_MAP_WRITE, 0, 0, 0);

	SetSpinCount(dwSpinCount);
	printf ("%d", m_psi->m_dwSpinCount);
}


InterProcessCS::~InterProcessCS()
{
	CloseHandle(m_hevt);
	UnmapViewOfFile(m_psi);
	CloseHandle(m_hfm);
}

void InterProcessCS::SetSpinCount(DWORD dwSpinCount)
{
		InterlockedExchangePointer((void**)&m_psi->m_dwSpinCount,
		(void*)(DWORD*)dwSpinCount);
}

void InterProcessCS::Enter() {

	if (TryEnter())
		return; 

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

BOOL InterProcessCS::TryEnter() {
	DWORD dwThreadId = GetCurrentThreadId();
	BOOL fThisThreadOwnsTheOptex = FALSE; 
	DWORD dwSpinCount = m_psi->m_dwSpinCount; 
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

	return(fThisThreadOwnsTheOptex);
}

void InterProcessCS::Leave() {

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