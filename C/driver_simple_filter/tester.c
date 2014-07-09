#include <stdio.h>
#include "public.h"

void push(HANDLE hFile, char input) {
	void* p = &input;
	printf("About to push: %d\n", input);

	long returnSize = 0;

	BOOLEAN success = DeviceIoControl(hFile,
								MY_IOCTL_PUSH,
								&p,
								sizeof(input),
								NULL,
								0,
								&returnSize,
								NULL);
	if (!success) {
		printf("Push error\n");
	} else {
		printf("Successfully pushed: %d\n", input);
	}
}

void pop(HANDLE hFile) {
	printf("About to pop\n");
	char output = 0;
	void* p = &output;
	long returnSize = 0;
	BOOLEAN success = DeviceIoControl(hFile,
								MY_IOCTL_POP,
								NULL,
								0,
								&p,
								sizeof(output),
								&returnSize,
								NULL);
	if (!success) {
		printf("Pop error\n");
	} else {
		printf("Successfully poped: %d\n", output);
	}
}

int main() {
	printf("Opening: " FILE_PATH "\n");

	HANDLE hFile = CreateFile(FILE_PATH, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if(hFile == INVALID_HANDLE_VALUE) {
		printf("Invalid handle\n");
		goto cleanup;
	}

	BOOLEAN success = TRUE;

	char input[1];
	char output[1];
	long returnSize = 0;

	push(hFile, 0);
	push(hFile, 5);
	push(hFile, 1);
	push(hFile, 10);
	push(hFile, 4);

	pop(hFile);

cleanup:
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
	return 0;
}