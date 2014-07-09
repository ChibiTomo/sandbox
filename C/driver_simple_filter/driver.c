#include <ntddk.h>

#include "driver.h"
#include "public.h"

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT  driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	DbgPrint("Load\n");

	driverObject->DriverUnload = my_unload;

	UNICODE_STRING deviceName;
	RtlInitUnicodeString(&deviceName, DEVICE_PATH);
	status = IoCreateDevice(driverObject,
							0,
							&deviceName,
							FILE_DEVICE_UNKNOWN,
							FILE_DEVICE_SECURE_OPEN,
							FALSE,
							&deviceObject);
	if (status != STATUS_SUCCESS) {
		DbgPrint("Cannot create device\n");
		goto cleanup;
	}

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		driverObject->MajorFunction[i] = my_unsuported_function;
	}
	driverObject->MajorFunction[IRP_MJ_CREATE] = my_create;
	driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = my_ioctl;

	deviceObject->Flags |= DO_BUFFERED_IO;
	deviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, DOSDEVICE_PATH);
	IoCreateSymbolicLink(&dosDeviceName, &deviceName);

cleanup:
	if (status != STATUS_SUCCESS) {
		if (deviceObject) {
			IoDeleteDevice(deviceObject);
		}
	}
	return status;
}

VOID STDCALL my_unload(PDRIVER_OBJECT driverObject) {
	DbgPrint("Unload\n");

	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, DOSDEVICE_PATH);
	IoDeleteSymbolicLink(&dosDeviceName);

	IoDeleteDevice(driverObject->DeviceObject);
}

NTSTATUS STDCALL my_unsuported_function(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_NOT_SUPPORTED;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("Unsuported function called without Irp location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	DbgPrint("Unsuported function: 0x%02X\n", pIoStackIrp->MajorFunction);

cleanup:
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_create(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_create called\n");

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	UINT dataSize = 0;

	DbgPrint("my_ioctl called\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("No I/O stack location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	DbgPrint("IOCTL = 0x%08X\n", pIoStackIrp->Parameters.DeviceIoControl.IoControlCode);
	switch(pIoStackIrp->Parameters.DeviceIoControl.IoControlCode) {
		case MY_IOCTL_PUSH:
			status = my_ioctl_push(Irp, &dataSize);
			break;

		case MY_IOCTL_POP:
			status = my_ioctl_pop(Irp, &dataSize);
			break;

		default:
			status = STATUS_NOT_SUPPORTED;
			break;
	}

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = dataSize;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS my_ioctl_push(PIRP Irp, UINT* dataRead) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_ioctl_push called\n");

	PCHAR c = (PCHAR) Irp->AssociatedIrp.SystemBuffer;

	DbgPrint("char received: %c\n", *c);
	*dataRead = 1;

	return status;
}

NTSTATUS my_ioctl_pop(PIRP Irp, UINT* dataWrite) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_ioctl_pop called\n");

	PCHAR c = (PCHAR) Irp->AssociatedIrp.SystemBuffer;

	*c = 'z';
	DbgPrint("char send: %c\n", *c);
	*dataWrite = 1;

	return status;
}