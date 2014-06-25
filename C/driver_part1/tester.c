#include <stdio.h>
#include <windows.h>

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
	printf("strlen(str) + 1 = %d\n", strlen(str) + 1);
	funcSuccess = WriteFile(hFile, str, strlen(str) + 1, &dwReturn, NULL); // return TRUE(non zero) on success
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

cleanup:
	if (hFile) {
		CloseHandle(hFile);
	}
	return status;
}