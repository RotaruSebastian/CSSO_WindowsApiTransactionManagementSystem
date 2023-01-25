#pragma once

#include <stdio.h>
#include <Windows.h>

#define DAILY "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Daily\\"
#define SHARED_MMAP "cssohw3management"
#define SHARED_LOGS "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\logs.txt"
#define SHARED_SUMMARY "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Summary\\summary.txt" 

#ifdef PAYMENTS
#define EXTENSION "_payments.txt"
#define SUMMARY "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Summary\\payments.txt"
#define INPUT_PATH "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\H3_input\\payments\\"
#define SIGN -1
#else
#define EXTENSION "_income.txt"
#define SUMMARY "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Summary\\income.txt"
#define INPUT_PATH "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\H3_input\\income\\"
#define SIGN 1
#endif

#define CHECK_ERROR(FCT,NAME,VAL)\
if(FCT == VAL){\
	printf("Err: " NAME " - %d\n", GetLastError());\
	return 0;\
}

int ReadDay(char* path, int* result);
int WriteDay(char* path, int result);
int WriteLog(int result, char* date);
int DaySummary(int result);
int DaySharedSummary(int result);
int DayValue(void* buffer, int* result);
int MappedValue(int sum);

HANDLE mutex_handle;
HANDLE semaphore_handle;
HANDLE event_handle;

int IterateFiles() {
	WIN32_FIND_DATA find_data;
	HANDLE find_handle;
	char path[MAX_PATH];
	int sum = 0;
	int result = 0;
	CHECK_ERROR((mutex_handle = OpenMutex(SYNCHRONIZE, FALSE, "csso_mutex")), "OpenMutex", NULL);
	CHECK_ERROR((semaphore_handle = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, "csso_semaphore")), "OpenSemaphore", NULL);
	CHECK_ERROR((event_handle = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, "csso_event")), "OpenEvent", NULL);
	CHECK_ERROR((find_handle = FindFirstFile(INPUT_PATH "*", &find_data)), "FindFirstFile", INVALID_HANDLE_VALUE);
	do {
		if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// citire pentru fiecare zi
			result = 0;
			strcpy(path, INPUT_PATH);
			strcat(path, find_data.cFileName);
			CHECK_ERROR(ReadDay(path, &result), "ReadDay", 0);
			// scriere pentru fiecare zi
			strcpy(path, DAILY);
			strcat(path, find_data.cFileName);
			strcat(path, EXTENSION);
			CHECK_ERROR(WriteDay(path, result), "WriteDay", 0);
			CHECK_ERROR(WriteLog(result, find_data.cFileName), "WriteLog", 0);
			// individual_summary
			CHECK_ERROR(DaySummary(result), "DaySummary", 0);
			// shared summary
			CHECK_ERROR(DaySharedSummary(result), "DaySharedSummary", 0);
			sum += result;
		}
	} while (FindNextFile(find_handle, &find_data) != 0);
	FindClose(find_handle);
	CloseHandle(mutex_handle);
	CloseHandle(semaphore_handle);
	CHECK_ERROR(MappedValue(sum), "MappedValue", 0);
	CloseHandle(event_handle);
	return 1;
}

int ReadDay(char* path, int* result) {
	HANDLE file_handle;
	file_handle = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	DWORD file_size = GetFileSize(file_handle, NULL);
	DWORD bytes_read;
	void *buffer = malloc(file_size + 1);
	CHECK_ERROR(ReadFile(file_handle, buffer, file_size, &bytes_read, NULL), "ReadFile", FALSE);
	*((char*)buffer + file_size) = '\0';
	CHECK_ERROR(DayValue(buffer,result), "DayValue", 0);
	CloseHandle(file_handle);
	free(buffer);
	return 1;
}

int WriteDay(char* path, int result) {
	HANDLE file_handle;
	file_handle = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	_itoa(result, path, 10);
	CHECK_ERROR(WriteFile(file_handle, path, (DWORD)strlen(path), NULL, NULL), "WriteFile", FALSE);
	CloseHandle(file_handle);
	return 1;
}

int WriteLog(int result, char* date) {
	HANDLE file_handle;
	char buffer[512];
	char number[10];
	if (SIGN == 1) {
		strcpy(buffer, "Am facut o incasare in valoare de ");
	}
	else {
		strcpy(buffer, "S-a facut o plata de ");
	}
	_itoa(result, number, 10);
	strcat(buffer, number);
	strcat(buffer, " in data de ");
	strcat(buffer, date);
	strcat(buffer, "\n");
	CHECK_ERROR(WaitForSingleObject(mutex_handle, INFINITE), "WaitForSingleObject", WAIT_FAILED);
	file_handle = CreateFile(SHARED_LOGS, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	CHECK_ERROR(SetFilePointer(file_handle, 0, NULL, FILE_END), "SetFilePointer", INVALID_SET_FILE_POINTER);
	CHECK_ERROR(WriteFile(file_handle, buffer, strlen(buffer), NULL, NULL), "WriteFile", FALSE);
	CloseHandle(file_handle);
	CHECK_ERROR(ReleaseMutex(mutex_handle), "ReleaseMutex", 0);
	return 1;
}

int DaySummary(int result) {
	HANDLE file_handle;
	DWORD bytes_read;
	char buffer[50];
	int old = 0;
	file_handle = CreateFile(SUMMARY, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	CHECK_ERROR(ReadFile(file_handle, buffer, 10, &bytes_read, NULL), "ReadFile", FALSE);
	buffer[bytes_read] = '\0';
	old = atoi(buffer);
	old += result;
	_itoa(old, buffer, 10);
	CHECK_ERROR(SetFilePointer(file_handle, 0, NULL, FILE_BEGIN), "SetFilePointer", INVALID_SET_FILE_POINTER);
	CHECK_ERROR(WriteFile(file_handle, buffer, (DWORD)strlen(buffer), NULL, NULL), "WriteFile", FALSE);
	CloseHandle(file_handle);
	return 1;
}

int DaySharedSummary(int result) {
	HANDLE file_handle;
	DWORD bytes_read;
	char buffer[21];
	int old = 0;
	CHECK_ERROR(WaitForSingleObject(semaphore_handle, INFINITE), "WaitForSingleObject", WAIT_FAILED);
	file_handle = CreateFile(SHARED_SUMMARY, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	CHECK_ERROR(ReadFile(file_handle, buffer, 20, &bytes_read, NULL), "ReadFile", FALSE);
	buffer[bytes_read] = '\0';
	old = atoi(buffer);
	old += SIGN * result;
	_itoa(old, buffer, 10);
	CHECK_ERROR(SetFilePointer(file_handle, 0, NULL, FILE_BEGIN), "SetFilePointer", INVALID_SET_FILE_POINTER);
	CHECK_ERROR(WriteFile(file_handle, buffer, (DWORD)strlen(buffer), NULL, NULL), "WriteFile", FALSE);
	CHECK_ERROR(SetEndOfFile(file_handle), "SetEndOfFile", FALSE);
	CloseHandle(file_handle);
	CHECK_ERROR(ReleaseSemaphore(semaphore_handle, 1, NULL), "ReleaseSemaphore", 0);
	return 1;
}

int DayValue(void* buffer, int* result) {
	char* token;
	token = strtok((char*)buffer, "\n");
	while (token != NULL) {
		*result += atoi(token);
		token = strtok(NULL, "\n");
	}
	return 1;
}

int MappedValue(int sum) {
	HANDLE map_handle;
	void* value;
	CHECK_ERROR(WaitForSingleObject(event_handle, INFINITE), "WaitForSingleObject", WAIT_FAILED);
	map_handle = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, SHARED_MMAP);
	CHECK_ERROR(map_handle, "OpenFileMapping", NULL);
	value = MapViewOfFile(map_handle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(int));
	CHECK_ERROR(value, "MapViewOfFile", NULL);
	*(int*)value += SIGN * sum;
	CHECK_ERROR(UnmapViewOfFile(value), "UnmapViewOfFile", FALSE);
	CHECK_ERROR(SetEvent(event_handle), "SetEvent", 0);
	return 1;
}
