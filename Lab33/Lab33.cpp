// Lab33.cpp: ���������� ����� ����� ��� ����������� ����������.
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
	printf("\nCreated first\n");
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
		ics->Enter();
		for (int i = 0; i < ARRAY_SIZE; i++)
		{
			values[i] = values[i] + 1;
		}
		for (int i = 0; i < ARRAY_SIZE; i++)
		{
			printf("%d ", values[i]);
		}
		printf("\n%d\n", GetCurrentProcessId());
		ics->Leave();
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
	
			if (hMapFile == NULL)
			{
				_tprintf(TEXT("Could not create file mapping object (%d).\n"),
					GetLastError());
				return 1;
			}
			pBuf = (LPTSTR)MapViewOfFile(hMapFile,
				FILE_MAP_ALL_ACCESS,
				0,
				0,
				BUF_SIZE);
	
			if (pBuf == NULL)
			{
				_tprintf(TEXT("Could not map view of file (%d).\n"),
					GetLastError());
	
				CloseHandle(hMapFile);
	
				return 1;
			}

			int values[ARRAY_SIZE];
			for (int i = 0; i < ARRAY_SIZE; i++)
			{
				values[i] = i;
			}	

			CopyMemory((PVOID)pBuf, values, sizeof(int) * ARRAY_SIZE);
			printf("\nCreated\n");
	
			STARTUPINFO si1;
			ZeroMemory(&si1, sizeof(STARTUPINFOA));
			PROCESS_INFORMATION pi1;
			CreateProcess(L"Lab33.exe", L" 1", NULL, NULL, FALSE, 0, NULL, NULL, &si1, &pi1);
	
			STARTUPINFO si2;
			ZeroMemory(&si2, sizeof(STARTUPINFOA));
			PROCESS_INFORMATION pi2;
			CreateProcess(L"Lab33.exe", L" 2", NULL, NULL, FALSE, 0, NULL, NULL, &si2, &pi2);
	
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

