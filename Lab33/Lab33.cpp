// Lab33.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include "InterProcessCS.h"
#define BUF_SIZE 256
#define ARRAY_SIZE 10
#define TRIES 5
WCHAR fileOfValuesName[] = L"Global\\Data";
InterProcessCS* ics;

void ProcessProcedure()
{
	HANDLE hMap;

	hMap = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		fileOfValuesName);

	int* values = (int*)MapViewOfFile(hMap,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BUF_SIZE);


	for (int j = 0; j < TRIES; j++)
	{
		ics->EnterCriticalSection();
		for (int i = 0; i < ARRAY_SIZE; i++)
		{
			values[i] = values[i] + 1;
		}
		for (int i = 0; i < ARRAY_SIZE; i++)
		{
			printf("%d ", values[i]);
		}
		printf("\n%d\n", GetCurrentProcessId());
		ics->LeaveCriticalSection();
		Sleep(10);
	}

	UnmapViewOfFile(values);
	CloseHandle(hMap);
}


int wmain(int argc, wchar_t *argv[])
{
	    ics = new InterProcessCS("Cross", 0);
		if (argc == 1)
		{
			HANDLE hMapFile;
			LPCTSTR pBuf;

			hMapFile = CreateFileMapping(
				INVALID_HANDLE_VALUE, 
				NULL,                   
				PAGE_READWRITE,          
				0,                      
				BUF_SIZE,                
				fileOfValuesName);

			pBuf = (LPTSTR)MapViewOfFile(hMapFile,
				FILE_MAP_ALL_ACCESS,
				0,
				0,
				BUF_SIZE);

			ics->EnterCriticalSection();
			ics->EnterCriticalSection();
			ics->EnterCriticalSection();
			ics->LeaveCriticalSection();
			ics->LeaveCriticalSection();
			ics->LeaveCriticalSection();

			int values[ARRAY_SIZE];
			for (int i = 0; i < ARRAY_SIZE; i++)
			{
				values[i] = i;
			}	

			CopyMemory((PVOID)pBuf, values, sizeof(int) * ARRAY_SIZE);
			printf("\nCreated\n");
	
			STARTUPINFO siFirst;
			ZeroMemory(&siFirst, sizeof(STARTUPINFOA));
			PROCESS_INFORMATION piFirst;
			CreateProcess(L"Lab33.exe", L" 1", NULL, NULL, FALSE, 0, NULL, NULL, &siFirst, &piFirst);
	
			STARTUPINFO siSecond;
			ZeroMemory(&siSecond, sizeof(STARTUPINFOA));
			PROCESS_INFORMATION piSecond;
			CreateProcess(L"Lab33.exe", L" 2", NULL, NULL, FALSE, 0, NULL, NULL, &siSecond, &piSecond);
	
			getch();
			UnmapViewOfFile(pBuf);
			CloseHandle(hMapFile);
			return 0;
	
		}
		else
		{ 
			ProcessProcedure();
			return 0;
		}

	return 0;
}

