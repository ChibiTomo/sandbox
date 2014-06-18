#include <ntddk.h>

VOID STDCALL my_unload(PDRIVER_OBJECT  DriverObject) {
	DbgPrint("GoodBye!!\n");
}

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS; // default status

	// Debug print
	DbgPrint("Hello!!\n");
	DbgPrint("RegistryPath=%wZ\n", registryPath);

	// Setting my_unload as unload function
	driverObject->DriverUnload = my_unload;

	return status;
}