#include <stdio.h>
#include <Windows.h>

#define INPUT_PATH "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\Daily\\"
#define LOGS "C:\\Facultate\\CSSO\\Laboratoare\\Week3\\Reports\\logs.txt"
#define MMAP "cssohw3management"

#define CHECK_ERROR(FCT,NAME,VAL)\
if(FCT == VAL){\
	printf("Err: " NAME " - %d\n", GetLastError());\
	return 0;\
}

int Validate();
int GetValue(char* path, int* value);
int WriteLogs(int income, int payments, int map_value);
int ReadFromMap();

int main() {
	CHECK_ERROR(Validate(), "Validate", 0);
}

int Validate() {
	WIN32_FIND_DATA find_data;
	HANDLE find_handle;
	char path[MAX_PATH];
	int result = 0;
	int income = 0;
	int payments = 0;
	int map_value = 0;
	CHECK_ERROR((find_handle = FindFirstFile(INPUT_PATH "*", &find_data)), "FindFirstFile", INVALID_HANDLE_VALUE);
	do {
		if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			result = 0;
			strcpy(path, INPUT_PATH);
			strcat(path, find_data.cFileName);
			if (strstr(find_data.cFileName, "income") != NULL) {
				CHECK_ERROR(GetValue(path, &income), "ReadDay", 0);
			}
			else {
				CHECK_ERROR(GetValue(path, &payments), "ReadDay", 0);
			}
		}
	} while (FindNextFile(find_handle, &find_data) != 0);
	FindClose(find_handle);
	CHECK_ERROR((map_value = ReadFromMap()), "ReadFromMap", 0);
	CHECK_ERROR(WriteLogs(income, payments, map_value), "WriteLogs", 0);
	return 1;
}

int ReadFromMap() {
	HANDLE map_handle;
	void* buffer;
	int number = 0;
	map_handle = OpenFileMapping(FILE_MAP_READ, FALSE, MMAP);
	CHECK_ERROR(map_handle, "OpenFileMapping", NULL);
	buffer = MapViewOfFile(map_handle, FILE_MAP_READ, 0, 0, sizeof(int));
	CHECK_ERROR(buffer, "MapViewOfFile", NULL);
	number = *(int*)buffer;
	CHECK_ERROR(UnmapViewOfFile(buffer), "UnmapViewOfFile", FALSE);
	CloseHandle(map_handle);
	return number;
}

int GetValue(char* path, int* value) {
	HANDLE file_handle;
	DWORD bytes_read;
	char buffer[11];
	file_handle = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	CHECK_ERROR(ReadFile(file_handle, buffer, 10, &bytes_read, NULL), "ReadFile", FALSE);
	buffer[bytes_read] = '\0';
	*value += atoi(buffer);
	CloseHandle(file_handle);
	return 1;
}

int WriteLogs(int income, int payments, int map_value) {
	HANDLE file_handle;
	char buffer[1000] = "S-au facut incasari de ";
	char number[11];
	_itoa(income, number, 10);
	strcat(buffer, number);
	strcat(buffer, "\nS-au facut plati de ");
	_itoa(payments, number, 10);
	strcat(buffer, number);
	strcat(buffer, "\nS-a realizat un profit de ");
	_itoa(income - payments, number, 10);
	strcat(buffer, number);
	if (income - payments == map_value) {
		strcat(buffer, "\nRaport generat cu succes!");
	}
	else {
		strcat(buffer, "\nAi o gresala la generarea raportului");
	}
	file_handle = CreateFile(LOGS, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CHECK_ERROR(file_handle, "CreateFile", INVALID_HANDLE_VALUE);
	CHECK_ERROR(SetFilePointer(file_handle, 0, NULL, FILE_END), "SetFilePointer", INVALID_SET_FILE_POINTER);
	CHECK_ERROR(WriteFile(file_handle, buffer, (DWORD)strlen(buffer), NULL, NULL), "WriteFile", FALSE);
	CloseHandle(file_handle);
	return 1;
}
