
#include "InterProcessCS.h"
#include "stdio.h"
#define BUFFER_OBJECT_NAME_SIZE 256
char* InterProcessCS::GenerateName(char* result, char* typeName, char* name) 
{
	result[0] = 0;
	wsprintfA(result, "%s%s", typeName, name);
	return(result);
}

InterProcessCS::InterProcessCS(char* name, DWORD spinCount)
{
	char result[BUFFER_OBJECT_NAME_SIZE];

	GenerateName(result, "FILE", name);
	mappedFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL,
		PAGE_READWRITE, 0, sizeof(*status), result);
	status = (STATUS*)MapViewOfFile(mappedFile,
		FILE_MAP_WRITE, 0, 0, 0);

	GenerateName(result, "EVENT", name);
	status->LockEvent = CreateEventA(NULL, FALSE, FALSE, result);

	status->LockCount = 0;
	status->OwningThread = 0;
	status->RecursionCount = 0;
	status->SpinCount = 0;

	SetSpinCount(spinCount);
}


InterProcessCS::~InterProcessCS()
{
	CloseHandle(status->LockEvent);
	UnmapViewOfFile(status);
	CloseHandle(mappedFile);
}

void InterProcessCS::SetSpinCount(DWORD spinCount)
{
	InterlockedExchange(&status->SpinCount, (LONG)spinCount);
}

void InterProcessCS::TakeByThread(DWORD threadId)
{
	InterlockedExchange(&status->OwningThread, threadId);
	InterlockedExchange(&status->RecursionCount, 1);
}

void InterProcessCS::EnterCriticalSection() 
{
	if (!TryEnterCriticalSection())
	{
		DWORD threadId = GetCurrentThreadId();
		if (InterlockedIncrement(&status->LockCount) == 1)
		{
			TakeByThread(threadId);
		}
		else
		{
			if (status->OwningThread == threadId)
			{
				InterlockedIncrement(&status->RecursionCount);
			}
			else
			{
				WaitForSingleObject(status->LockEvent, INFINITE);
				TakeByThread(threadId);
			}
		}
	}
}

BOOL InterProcessCS::TryEnterCriticalSection() 
{
	DWORD dwThreadId = GetCurrentThreadId();
	BOOL isCurrentThreadOwn = FALSE; 
	DWORD SpinCount = status->SpinCount; 
	do {
		if (InterlockedCompareExchange(&status->LockCount, 1, 0) == 0)
		{
			isCurrentThreadOwn = TRUE;
		}
		if (isCurrentThreadOwn)
		{
			TakeByThread(dwThreadId);
		}
		else 
		{
			if (status->OwningThread == dwThreadId)
			{
				InterlockedIncrement(&status->LockCount);
				InterlockedIncrement(&status->RecursionCount);
				isCurrentThreadOwn = TRUE;
			}
		}
		SpinCount--;
	} while (!isCurrentThreadOwn && (SpinCount > 0));

	return(isCurrentThreadOwn);
}

void InterProcessCS::LeaveCriticalSection()
{
	if (status->OwningThread == GetCurrentThreadId())
	{
		if (InterlockedDecrement(&status->RecursionCount) <= 0)
		{
			status->OwningThread = 0;
			if (InterlockedDecrement(&status->LockCount) > 0)
			{
				SetEvent(status->LockEvent);
			}
		}
		else
		{
			InterlockedDecrement(&status->LockCount);
		}
	}	
}