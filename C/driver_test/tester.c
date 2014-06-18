#include <stdio.h>
#include <windows.h>

int main(void) {
	HANDLE hFile;
	DWORD dwReturn;

	hFile = CreateFile( "\\\\.\\Example",
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

	if(hFile) {
		WriteFile(hFile, "Hello from user mode!\n", sizeof("Hello from user mode!\n"), &dwReturn, NULL);
		CloseHandle(hFile);
	}

	return 0;
}