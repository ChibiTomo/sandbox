#include <stdio.h>
#include "public.h"

void push(HANDLE hFile, char input) {
	printf("About to push: %c\n", input);

	long returnSize = 0;

	BOOLEAN success = DeviceIoControl(hFile,
								MY_IOCTL_PUSH,
								&input,
								sizeof(input),
								NULL,
								0,
								&returnSize,
								NULL);
	if (!success) {
		printf("Push error\n");
	} else {
		printf("Successfully pushed: %c\n", input);
	}
}

void pop(HANDLE hFile) {
	printf("About to pop\n");
	char output = 0;
	long returnSize = 0;
	BOOLEAN success = DeviceIoControl(hFile,
								MY_IOCTL_POP,
								NULL,
								0,
								&output,
								sizeof(output),
								&returnSize,
								NULL);
	if (!success) {
		printf("Pop error\n");
	} else {
		printf("Successfully poped: %c\n", output);
	}
}

int main() {
	int result = 0;

	printf("Opening: " FILE_PATH "\n");

	HANDLE hFile = CreateFile(FILE_PATH, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if(hFile == INVALID_HANDLE_VALUE) {
		printf("Invalid handle\n");
		result = 1;
		goto cleanup;
	}

	push(hFile, 'a');
	push(hFile, 'g');
	push(hFile, 'A');
	push(hFile, 't');
	push(hFile, 145);

	pop(hFile);

cleanup:
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
	return result;
}