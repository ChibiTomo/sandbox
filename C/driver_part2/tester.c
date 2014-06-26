#include <stdio.h>
#include <windows.h>

#include "public.h"

#define BUFFER_SIZE 1024

int main(void) {
	int status = 0;
	HANDLE hFile = NULL;
	DWORD dwReturn;
	char buf[BUFFER_SIZE];
	int funcSuccess = 1;

	hFile = CreateFile( "\\\\.\\Example",
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

	if(hFile == INVALID_HANDLE_VALUE) {
		goto cleanup;
	}

	// IO Control
	printf("\n---- Testing Io Control\n");

	memset(buf, BUFFER_SIZE, 0);
	char* str1 = "** Hello from User Mode Direct OUT I/O";
	funcSuccess = DeviceIoControl(hFile,
									IOCTL_DIRECT_OUT_IO,
									str1,
									strlen(str1) + 1,
									buf,
									BUFFER_SIZE,
									&dwReturn,
									NULL);
	if (!funcSuccess) {
		printf("IOCTL_DIRECT_OUT_IO error...\n");
		goto cleanup;
	}
	printf("%.*s\n", dwReturn, buf);

	ZeroMemory(buf, BUFFER_SIZE);
	char* str2 = "** Hello from User Mode Direct IN I/O";
	funcSuccess = DeviceIoControl(hFile,
									IOCTL_DIRECT_IN_IO,
									str2,
									strlen(str2) + 1,
									buf,
									BUFFER_SIZE,
									&dwReturn,
									NULL);
	if (!funcSuccess) {
		printf("IOCTL_DIRECT_IN_IO error...\n");
		goto cleanup;
	}
	printf("%.*s\n", dwReturn, buf);

	memset(buf, BUFFER_SIZE, 0);
	char* str3 = "** Hello from User Mode Buffered I/O";
	funcSuccess = DeviceIoControl(hFile,
									IOCTL_BUFFERED_IO,
									str3,
									strlen(str3) + 1,
									buf,
									BUFFER_SIZE,
									&dwReturn,
									NULL);
	if (!funcSuccess) {
		printf("IOCTL_BUFFERED_IO error...\n");
		goto cleanup;
	}
	printf("%.*s\n", dwReturn, buf);

cleanup:
	if (hFile) {
		CloseHandle(hFile);
	}
	return status;
}