#include <stdio.h>
#include "public.h"

#define PUSH(nbr) \
	input[0] = nbr; \
	printf("About to push: %d\n", input[0]); \
	ZeroMemory(output, sizeof(output)); \
	success = DeviceIoControl(hFile, \
								MY_IOCTL_PUSH, \
								&input, \
								sizeof(input), \
								NULL, \
								0, \
								&returnSize, \
								NULL); \
	if (!success) { \
		printf("Push error\n"); \
	} else { \
		printf("Successfully pushed: %d\n", input[0]); \
	}

#define POP() \
	input[0] = 0; \
	ZeroMemory(output, sizeof(output)); \
	success = DeviceIoControl(hFile, \
								MY_IOCTL_POP, \
								&input, \
								sizeof(input), \
								&output, \
								sizeof(output), \
								&returnSize, \
								NULL); \
	if (!success) { \
		printf("Pop error\n"); \
		goto cleanup; \
	} \
	printf("Popping: %d\n", output[0]);


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

	PUSH(0);
	PUSH(5);
	PUSH(1);
	PUSH(10);
	PUSH(4);

	POP();

cleanup:
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
	return 0;
}