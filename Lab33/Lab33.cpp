// Lab33.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include "Optex.h"
#define BUF_SIZE 256
TCHAR szName[] = TEXT("Global\\O");
TCHAR szMsg[] = TEXT("Message from first process.");

int wmain(int argc, wchar_t *argv[])
{
	COptex* optex = new COptex();

	optex->Enter();
	optex->Enter(); // проверяем "рекурсию захватов"

					// покидаем оптекс, но он все еще принадлежит нам
	optex->Leave();

	optex->Leave(); // теперь первичный поток может работать

	return 0;
}

//int wmain(int argc, wchar_t *argv[])
//{
//	if (argc == 1)
//	{
//		HANDLE hMapFile;
//		LPCTSTR pBuf;
//
//		hMapFile = CreateFileMapping(
//			INVALID_HANDLE_VALUE,    // use paging file
//			NULL,                    // default security
//			PAGE_READWRITE,          // read/write access
//			0,                       // maximum object size (high-order DWORD)
//			BUF_SIZE,                // maximum object size (low-order DWORD)
//			szName);                 // name of mapping object
//
//		if (hMapFile == NULL)
//		{
//			_tprintf(TEXT("Could not create file mapping object (%d).\n"),
//				GetLastError());
//			return 1;
//		}
//		pBuf = (LPTSTR)MapViewOfFile(hMapFile,   // handle to map object
//			FILE_MAP_ALL_ACCESS, // read/write permission
//			0,
//			0,
//			BUF_SIZE);
//
//		if (pBuf == NULL)
//		{
//			_tprintf(TEXT("Could not map view of file (%d).\n"),
//				GetLastError());
//
//			CloseHandle(hMapFile);
//
//			return 1;
//		}
//
//		CRITICAL_SECTION cs;
//		InitializeCriticalSection(&cs);
//
//		CopyMemory((PVOID)pBuf, &cs, sizeof(cs));
//
//		printf("Created\n");
//
//		STARTUPINFO si1;
//		ZeroMemory(&si1, sizeof(STARTUPINFOA));
//		PROCESS_INFORMATION pi1;
//		CreateProcess(L"Lab3.exe", L" 1", NULL, NULL, FALSE, 0, NULL, NULL, &si1, &pi1);
//
//		STARTUPINFO si2;
//		ZeroMemory(&si2, sizeof(STARTUPINFOA));
//		PROCESS_INFORMATION pi2;
//		CreateProcess(L"Lab3.exe", L" 2", NULL, NULL, FALSE, 0, NULL, NULL, &si2, &pi2);
//
//		getch();
//		UnmapViewOfFile(pBuf);
//
//		CloseHandle(hMapFile);
//		return 0;
//
//	}
//	else
//	{ 
//		if (wcscmp(argv[1], L"1") == 0)
//		{
//			printf("Created first\n");
//			HANDLE hMap;
//			CRITICAL_SECTION* cs1;
//			cs1 = (CRITICAL_SECTION*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE);
//			EnterCriticalSection(cs1);
//			//LeaveCriticalSection(cs1);
//		}
//		else
//		{
//			printf("Created second\n");
//			HANDLE hMap;
//			CRITICAL_SECTION* cs2;
//			cs2 = (CRITICAL_SECTION*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, BUF_SIZE);
//			EnterCriticalSection(cs2);
//			//LeaveCriticalSection(cs2);
//		}
//		getch();
//		return 0;
//	}
//
//
//	
//}

