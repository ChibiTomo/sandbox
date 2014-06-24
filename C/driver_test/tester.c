#include <stdio.h>
#include <windows.h>

#define BUFFER_SIZE 1024

int main(void) {
	int status = 0;
	HANDLE hFile;
	DWORD dwReturn;
	char buf[BUFFER_SIZE];

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
	status = !WriteFile(hFile, &str, strlen(str), &dwReturn, NULL); // return TRUE(non zero) on success
	if (status != 0) {
		printf("Write error...\n");
		goto cleanup;
	}
	printf("bytes written: %d\n", dwReturn);

	dwReturn = 0;
	status = !ReadFile(hFile, buf, BUFFER_SIZE, &dwReturn, NULL); // return TRUE(non zero) on success
	if (status != 0) {
		printf("Read error...\n");
		goto cleanup;
	}
	printf("bytes readed: %d\n", dwReturn);
	printf("Kernel says: %.*s\n", dwReturn, buf);

	CloseHandle(hFile);

cleanup:
	return status;
}