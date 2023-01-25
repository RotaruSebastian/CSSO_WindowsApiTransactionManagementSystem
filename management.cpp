#include <stdio.h>
#include <Windows.h>

#define DIRECTORY_PATH "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\"
#define PAY_EXE "C:\\Users\\Rotaru\\Documents\\FAC\\csso\\pay\\x64\\Debug\\pay.exe"
#define INCOME_EXE "C:\\Users\\Rotaru\\Documents\\FAC\\csso\\income\\x64\\Debug\\income.exe"
#define GENERATE_EXE "C:\\Users\\Rotaru\\Documents\\FAC\\csso\\generate\\x64\\Debug\\generate.exe"
#define SHARED_MMAP "cssohw3management"

#define CHECK_ERROR(FCT,NAME,VAL)\
if(FCT == VAL){\
	printf("Err: " NAME " - %d\n", GetLastError());\
	return 0;\
}

int CreateDirs(const char* path);
int InitMap(HANDLE * map_handle, void* buffer);
int CreateProcessFiles();
int LaunchProcesses();
int ReadFiles();
int ReadMap(HANDLE * map_handle, void* buffer);
void InitStructs(STARTUPINFO * startup_info, PROCESS_INFORMATION * process_information);

int main() {
	HANDLE map_handle;
	void* buffer = NULL;

	CHECK_ERROR(CreateDirs(DIRECTORY_PATH "Daily"), "CreateDirs", 0);
	CHECK_ERROR(CreateDirs(DIRECTORY_PATH "Summary"), "CreateDirs", 0);

	CHECK_ERROR(InitMap(&map_handle, buffer), "InitMap", 0);

	CHECK_ERROR(LaunchProcesses(), "LaunchProcesses", 0);

	CHECK_ERROR(ReadFiles(), "ReadFiles", 0);

	CHECK_ERROR(ReadMap(&map_handle, buffer), "ReadMap", 0);
}

int CreateDirs(const char* path) {
	char copy1[MAX_PATH];
	char copy2[MAX_PATH];
	char* token;
	memset(copy1, 0, MAX_PATH);
	memset(copy2, 0, MAX_PATH);
	strcpy(copy1, path);
	token = strtok(copy1, "\\");
	strcat(copy2, token);
	while ((token = strtok(NULL, "\\")) != NULL) {
		strcat(copy2, "\\");
		strcat(copy2, token);
		if (CreateDirectory(copy2, NULL) == 0) {
			int error = GetLastError();
			if (error != ERROR_ALREADY_EXISTS) {
				printf("Err: CreateDirectory - %d\n", error);
				return 0;
			}
		}
	}
	return 1;
}

int InitMap(HANDLE* map_handle, void* buffer) {
	int value = 0;
	*map_handle = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(int),
		SHARED_MMAP
	);
	CHECK_ERROR(*map_handle, "CreateFileMapping", NULL);
	CHECK_ERROR((buffer = MapViewOfFile(*map_handle, FILE_MAP_WRITE, 0, 0, sizeof(int))), "MapViewOfFile", NULL);
	CopyMemory(buffer, &value, sizeof(int));
	CHECK_ERROR(UnmapViewOfFile(buffer), "UnmapViewOfFile", FALSE);
	return 1;
}

int CreateProcessFiles() {
	HANDLE file_handle;
	const char* buffer = "0";
	file_handle = CreateFile(DIRECTORY_PATH "logs.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	CloseHandle(file_handle);
	file_handle = CreateFile(DIRECTORY_PATH "\\Summary\\payments.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	CHECK_ERROR(WriteFile(file_handle, buffer, 1, NULL, NULL), "WriteFile", FALSE);
	CloseHandle(file_handle);
	file_handle = CreateFile(DIRECTORY_PATH "\\Summary\\income.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	CHECK_ERROR(WriteFile(file_handle, buffer, 1, NULL, NULL), "WriteFile", FALSE);
	CloseHandle(file_handle);
	file_handle = CreateFile(DIRECTORY_PATH "\\Summary\\summary.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	CHECK_ERROR(WriteFile(file_handle, buffer, 1, NULL, NULL), "WriteFile", FALSE);
	CloseHandle(file_handle);
	return 1;
}

void InitStructs(STARTUPINFO* startup_info, PROCESS_INFORMATION* process_information) {
	memset(startup_info, 0, sizeof(STARTUPINFO));
	startup_info->cb = sizeof(STARTUPINFO);
	memset(process_information, 0, sizeof(PROCESS_INFORMATION));
}

int LaunchProcesses() {
	HANDLE mutex_handle;
	HANDLE semaphore_handle;
	HANDLE event_handle;
	HANDLE handles[2];
	STARTUPINFO pay_startup_info, income_startup_info, generate_startup_info;
	PROCESS_INFORMATION pay_process_information, income_process_information, generate_process_information;
	InitStructs(&pay_startup_info, &pay_process_information);
	InitStructs(&income_startup_info, &income_process_information);
	InitStructs(&generate_startup_info, &generate_process_information);
	CHECK_ERROR(CreateProcessFiles(), "CreateProcessFiles", 0);
	// pay.exe & income.exe
	CHECK_ERROR((mutex_handle = CreateMutex(NULL, FALSE, "csso_mutex")), "CreateMutex", NULL);
	CHECK_ERROR((semaphore_handle = CreateSemaphore(NULL, 1, 1, "csso_semaphore")), "CreateSemaphore", NULL);
	CHECK_ERROR((event_handle = CreateEvent(NULL, FALSE, TRUE, "csso_event")), "CreateEvent", NULL);
	if (!CreateProcess(PAY_EXE, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &pay_startup_info, &pay_process_information)) {
		printf("Err: CreateProcess (pay) - %d\n", GetLastError());
		return 0;
	}
	if (!CreateProcess(INCOME_EXE, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &income_startup_info, &income_process_information)) {
		printf("Err: CreateProcess (income) - %d\n", GetLastError());
		return 0;
	}
	handles[0] = pay_process_information.hProcess;
	handles[1] = income_process_information.hProcess;
	CHECK_ERROR(WaitForMultipleObjects(2, handles, TRUE, 60000), "WaitForMultipleObjects", WAIT_FAILED);
	CloseHandle(pay_process_information.hProcess);
	CloseHandle(pay_process_information.hThread);
	CloseHandle(income_process_information.hProcess);
	CloseHandle(income_process_information.hThread);
	// generate.exe
	if (!CreateProcess(GENERATE_EXE, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &generate_startup_info, &generate_process_information)) {
		printf("Err: CreateProcess (generate) - %d\n", GetLastError());
		return 0;
	}
	CHECK_ERROR(WaitForSingleObject(generate_process_information.hProcess, 60000), "WaitForSingleObject", WAIT_FAILED);
	CloseHandle(generate_process_information.hProcess);
	CloseHandle(generate_process_information.hThread);
	CloseHandle(mutex_handle);
	CloseHandle(semaphore_handle);
	CloseHandle(event_handle);
	return 1;
}

int ReadFiles() {
	HANDLE file_handle;
	DWORD bytes_read;
	char buffer[50];
	memset(buffer, 0, 50);

	file_handle = CreateFile(DIRECTORY_PATH "Summary\\income.txt", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	CHECK_ERROR(ReadFile(file_handle, buffer, 50, &bytes_read, NULL), "ReadFile", FALSE);
	printf("income.txt: %s\n", buffer);
	CloseHandle(file_handle);
	memset(buffer, 0, 50);

	file_handle = CreateFile(DIRECTORY_PATH "Summary\\payments.txt", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	CHECK_ERROR(ReadFile(file_handle, buffer, 50, &bytes_read, NULL), "ReadFile", FALSE);
	printf("payments.txt: %s\n", buffer);
	CloseHandle(file_handle);
	memset(buffer, 0, 50);

	file_handle = CreateFile(DIRECTORY_PATH "Summary\\summary.txt", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	CHECK_ERROR(ReadFile(file_handle, buffer, 50, &bytes_read, NULL), "ReadFile", FALSE);
	printf("summary.txt: %s\n", buffer);
	CloseHandle(file_handle);
	return 1;
}

int ReadMap(HANDLE* map_handle, void* buffer) {
	buffer = MapViewOfFile(*map_handle, FILE_MAP_READ, 0, 0, sizeof(int));
	CHECK_ERROR(buffer, "MapViewOfFile", NULL);
	printf("profit: %d", *(int*)buffer);
	CHECK_ERROR(UnmapViewOfFile(buffer), "UnmapViewOfFile", FALSE);
	CloseHandle(*map_handle);
	return 1;
}
