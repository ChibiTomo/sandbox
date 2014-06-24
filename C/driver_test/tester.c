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
	char* str = "Hello from user mode!\n";
	funcSuccess = WriteFile(hFile, &str, strlen(str), &dwReturn, NULL); // return TRUE(non zero) on success
	if (!funcSuccess) {
		printf("Write error...\n");
		goto cleanup;
	}
	printf("bytes written: %d\n", dwReturn);

	dwReturn = 0;
	funcSuccess = ReadFile(hFile, buf, BUFFER_SIZE, &dwReturn, NULL); // return TRUE(non zero) on success
	if (!funcSuccess) {
		printf("Read error...\n");
		goto cleanup;
	}
	printf("bytes readed: %d\n", dwReturn);
	printf("Kernel says: %.*s\n", dwReturn, buf);


	// IO Control
	printf("\n---- Testing Io Control\n");

	memset(buf, BUFFER_SIZE, 0);
	funcSuccess = DeviceIoControl(hFile,
									IOCTL_DIRECT_OUT_IO,
									"** Hello from User Mode Direct OUT I/O",
									sizeof("** Hello from User Mode Direct OUT I/O"),
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
	funcSuccess = DeviceIoControl(hFile,
									IOCTL_DIRECT_IN_IO,
									"** Hello from User Mode Direct IN I/O",
									sizeof("** Hello from User Mode Direct IN I/O"),
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
	funcSuccess = DeviceIoControl(hFile,
									IOCTL_BUFFERED_IO,
									"** Hello from User Mode Buffered I/O",
									sizeof("** Hello from User Mode Buffered I/O"),
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