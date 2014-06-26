#include <ntddk.h>

#include "driver.h"
#include "public.h"

#define DbgPrint(format, ...) DbgPrint("Driver: "format, ##__VA_ARGS__)


NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS;

	driverObject->DriverUnload = my_unload;

	DbgPrint("Loaded.\n");

	UNICODE_STRING deviceName;
	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&deviceName, DEVICE_PATH);
	RtlInitUnicodeString(&dosDeviceName, DOSDEVICE_PATH);

	PDEVICE_OBJECT deviceObject = NULL;
	status = IoCreateDevice(driverObject,
							sizeof(device_context_t),
							&deviceName,
							FILE_DEVICE_UNKNOWN,
							FILE_DEVICE_SECURE_OPEN,
							FALSE,
							&deviceObject);

	if (status != STATUS_SUCCESS) {
		IoDeleteDevice(deviceObject);
		goto cleanup;
	}

	for (UINT i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		driverObject->MajorFunction[i] = unsuported_function;
	}
	driverObject->MajorFunction[IRP_MJ_CLOSE] = my_close;
	driverObject->MajorFunction[IRP_MJ_CREATE] = my_create;

	device_context_t* context = (device_context_t*) deviceObject->DeviceExtension;

	KeInitializeMutex(&context->mutex, 0);
	context->info_list = NULL;

	deviceObject->Flags |= DO_BUFFERED_IO;
	deviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

	IoCreateSymbolicLink(&dosDeviceName, &deviceName);

cleanup:
	return status;
}

VOID STDCALL my_unload(PDRIVER_OBJECT DriverObject) {
	DbgPrint("GoodBye!!\n");

	// Remove SymLinks
	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, DOSDEVICE_PATH);
	IoDeleteSymbolicLink(&dosDeviceName);

	// Remove device
	PDEVICE_OBJECT device = DriverObject->DeviceObject;
	if (device != NULL) {
		IoDeleteDevice(device);
	}
}

NTSTATUS STDCALL unsuported_function(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_NOT_SUPPORTED;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	DbgPrint("unsuported_function called: 0x%02X\n", pIoStackIrp->MajorFunction);

	return status;
}

NTSTATUS STDCALL my_create(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("my_create called \n");

	PIO_STACK_LOCATION pIoStackIrp = NULL;
	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);

	//status = createPipeContext((device_context_t*) DeviceObject->DeviceExtension, pIoStackIrp->FileObject);
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS STDCALL my_close(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("my_close called \n");

	PIO_STACK_LOCATION pIoStackIrp = NULL;
	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);

	//status = releasePipeContext((device_context_t*) DeviceObject->DeviceExtension, pIoStackIrp->FileObject);
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS createPipeContext(device_context_t* deviceContext, PFILE_OBJECT pFileObject) {
}