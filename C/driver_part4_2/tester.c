#include <stdio.h>
#include "public.h"

void say(HANDLE hFile, DWORD ioControlCode) {
	long returnSize = 0;
	BOOLEAN success = DeviceIoControl(hFile,
								ioControlCode,
								NULL,
								0,
								NULL,
								0,
								&returnSize,
								NULL);
	if (!success) {
		printf("Discussion error\n");
	} else {
		printf("Everyone is polite\n");
	}
}

void sayHello(HANDLE first, HANDLE second) {
	printf("First driver says hello to second\n");
	say(first, MY_IOCTL_SAY_HELLO);

	printf("Second driver says hello to first\n");
	say(second, MY_IOCTL_SAY_HELLO);
}

void sayGoodBye(HANDLE first, HANDLE second) {
	printf("First driver says goodbye to second\n");
	say(first, MY_IOCTL_SAY_GOODBYE);

	printf("Second driver says goodbye to first\n");
	say(second, MY_IOCTL_SAY_GOODBYE);
}

int main() {
	int result = 0;
	HANDLE firstFile = INVALID_HANDLE_VALUE;
	HANDLE secondFile = INVALID_HANDLE_VALUE;

	printf("Opening: " FIRST_FILE_PATH "\n");
	firstFile = CreateFile(FIRST_FILE_PATH, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(firstFile == INVALID_HANDLE_VALUE) {
		printf("Invalid handle\n");
		result = 1;
		goto cleanup;
	}

	printf("Opening: " SECOND_FILE_PATH "\n");
	secondFile = CreateFile(SECOND_FILE_PATH, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(secondFile == INVALID_HANDLE_VALUE) {
		printf("Invalid handle\n");
		result = 1;
		goto cleanup;
	}

	sayHello(firstFile, secondFile);
	sayGoodBye(firstFile, secondFile);


cleanup:
	if (firstFile != INVALID_HANDLE_VALUE) {
		CloseHandle(firstFile);
	}
	if (secondFile != INVALID_HANDLE_VALUE) {
		CloseHandle(secondFile);
	}
	return result;
}