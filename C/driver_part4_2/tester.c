#include <stdio.h>
#include "public.h"

int main() {
	int result = 0;

	printf("Opening: " SECOND_FILE_PATH "\n");
	HANDLE hFile = CreateFile(SECOND_FILE_PATH, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE) {
		printf("Invalid handle\n");
		result = 1;
		goto cleanup;
	}

	long returnSize = 0;
	BOOLEAN success = DeviceIoControl(hFile,
								MY_IOCTL_SAY_HELLO,
								NULL,
								0,
								NULL,
								0,
								&returnSize,
								NULL);
	if (success) {
		printf("DeviceIoControl: success\n");
	} else {
		printf("DeviceIoControl: error\n");
	}

cleanup:
	if (hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
	}
	return result;
}