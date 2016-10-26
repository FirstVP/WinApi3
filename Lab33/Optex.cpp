/******************************************************************************
Модуль: Optex.cpp
Автор: Copyright (c) 2000, Джеффри Рихтер (Jeffrey Richter)
******************************************************************************/
//#include "..\CmnHdr.h" /* см. приложение A */
#include "Optex.h"
///////////////////////////////////////////////////////////////////////////////
// 0=многопроцессорная машина, 1=однопроцессорная, -1=тип пока не определен
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
		// конструируется первый объект; выясняем количество процессоров
		SYSTEM_INFO sinf;
		GetSystemInfo(&sinf);
		sm_fUniprocessorHost = (sinf.dwNumberOfProcessors == 1);
	}
	m_hevt = m_hfm = NULL;
	m_psi = NULL;
	if (pszName == NULL) { // создание однопроцессного оптекса
		m_hevt = CreateEventA(NULL, FALSE, FALSE, NULL);
		//chASSERT(m_hevt != NULL);
		m_psi = new SHAREDINFO;
		//chASSERT(m_psi != NULL);
		ZeroMemory(m_psi, sizeof(*m_psi));
	}
	else { // создание межпроцессного оптекса
		   // всегда используем ANSI, чтобы программа работала в Win9x и Windows 2000
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
		// Примечание: элементы m_lLockCount, m_dwThreadId и m_lRecurseCount
		// структуры SHAREDINFO надо инициализировать нулевым значением. К счастью,
		// механизм проецирования файлов в память избавляет нас от части работы,
		// связанной с синхронизацией потоков.
	}
	SetSpinCount(dwSpinCount);
}
///////////////////////////////////////////////////////////////////////////////
COptex::~COptex() {
#ifdef _DEBUG
	if (IsSingleProcessOptex() && (m_psi->m_dwThreadId != 0)) {
		// однопроцессный оптекс нельзя разрушать,
		// если им владеет какой-нибудь поток
		DebugBreak();
	}
	if (!IsSingleProcessOptex() &&
		(m_psi->m_dwThreadId == GetCurrentThreadId())) {
		// межпроцессный оптекс нельзя разрушать, если им владеет наш поток
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
	// спин-блокировка на однопроцессорных машинах не применяется
	if (!sm_fUniprocessorHost)
		InterlockedExchangePointer((PVOID*)&m_psi->m_dwSpinCount,
		(PVOID)(DWORD_PTR)dwSpinCount);
}
///////////////////////////////////////////////////////////////////////////////
void COptex::Enter() {
	// "крутимся", пытаясь захватить оптекс
	if (TryEnter())
		return; // получилось, возвращаем управление
				// захватить оптекс не удалось, переходим в состояние ожидания
	DWORD dwThreadId = GetCurrentThreadId();
	if (InterlockedIncrement(&m_psi->m_lLockCount) == 1) {
		// оптекс не занят, пусть этот поток захватит его разок
		m_psi->m_dwThreadId = dwThreadId;
		m_psi->m_lRecurseCount = 1;
	}
	else {
		if (m_psi->m_dwThreadId == dwThreadId) {
			// если оптекс принадлежит данному потоку, захватываем его еще раз
			m_psi->m_lRecurseCount++;
		}
		else {
			// оптекс принадлежит другому потоку, ждем
			WaitForSingleObject(m_hevt, INFINITE);
			// оптекс не занят, пусть этот поток захватит его разок
			m_psi->m_dwThreadId = dwThreadId;
			m_psi->m_lRecurseCount = 1;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
BOOL COptex::TryEnter() {
	DWORD dwThreadId = GetCurrentThreadId();
	BOOL fThisThreadOwnsTheOptex = FALSE; // считаем, что поток владеет оптексом
	DWORD dwSpinCount = m_psi->m_dwSpinCount; // задаем число циклов
	do {
		// если счетчик числа блокировок = 0, оптекс не занят,
		// и мы можем захватить его
		fThisThreadOwnsTheOptex = (0 ==
			InterlockedCompareExchange(&m_psi->m_lLockCount, 1, 0));
		if (fThisThreadOwnsTheOptex) {
			// оптекс не занят, пусть этот поток захватит его разок
			m_psi->m_dwThreadId = dwThreadId;
			m_psi->m_lRecurseCount = 1;
		}
		else {
			if (m_psi->m_dwThreadId == dwThreadId) {
				// если оптекс принадлежит данному потоку, захватываем его еще раз
				InterlockedIncrement(&m_psi->m_lLockCount);
				m_psi->m_lRecurseCount++;
				fThisThreadOwnsTheOptex = TRUE;
			}
		}
	} while (!fThisThreadOwnsTheOptex && (dwSpinCount-- > 0));
	// возвращаем управление независимо от того,
	// владеет данный поток оптексом или нет
	return(fThisThreadOwnsTheOptex);
}
///////////////////////////////////////////////////////////////////////////////
void COptex::Leave() {
#ifdef _DEBUG
	// покинуть оптекс может лишь тот поток, который им владеет
	if (m_psi->m_dwThreadId != GetCurrentThreadId())
		DebugBreak();
#endif
	// уменьшаем счетчик числа захватов оптекса данным потоком
	if (--m_psi->m_lRecurseCount > 0) {
		// оптекс все еще принадлежит нам
		InterlockedDecrement(&m_psi->m_lLockCount);
	}
	else {
		// оптекс нам больше не принадлежит
		m_psi->m_dwThreadId = 0;
		if (InterlockedDecrement(&m_psi->m_lLockCount) > 0) {
			// если оптекс ждут другие потоки,
			// событие с автосбросом пробудит один из них
			SetEvent(m_hevt);
		}
	}
}