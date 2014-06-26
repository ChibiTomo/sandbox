#include <ntddk.h>

void DriverUnload(PDRIVER_OBJECT pDriverObject) {
	DbgPrint("Driver unloading\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
//	int x = 0;
//	int y = 0;

	DriverObject->DriverUnload = DriverUnload;
	__try {
		DbgPrint("Hello, World\n");
		ProbeForRead(0x00400000, 1, 1);
//		y = x/x;
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		DbgPrint("Error\n");
	}
	return STATUS_SUCCESS;
}