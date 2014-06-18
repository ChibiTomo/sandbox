#include <stdio.h>
#include <windows.h>
#include <setjmp.h>

jmp_buf g_ex_buf__;

LONG WINAPI my_handler(LPEXCEPTION_POINTERS exceptionInfo) {
	printf("Handle exception\n");
	longjmp(g_ex_buf__, -1);
	return EXCEPTION_CONTINUE_EXECUTION;
}

void may_fail(int b) {
	if (b) {
		int i = 10;
		int *p = (int *) i;
		printf("address pointed by p = 0x%08x\n", *p);
	}
}

int try_may_fail(int b) {
	if (setjmp(g_ex_buf__) == 0) {
		SetUnhandledExceptionFilter(my_handler);
		may_fail(b);
	} else {
		SetUnhandledExceptionFilter(NULL);
		return 0;
	}
	return 1;
}

int main() {
	int result = 0;
	result = try_may_fail(1);
	printf("result = %d\n", result);
	result = try_may_fail(0);
	printf("result = %d\n", result);

	return 0;
}