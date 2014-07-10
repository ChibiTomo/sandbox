#include <ntddk.h>
#include "public.h"
#include "first_driver.h"

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	DbgPrint("Load\n");

	driverObject->DriverUnload = my_unload;

	UNICODE_STRING deviceName;
	RtlInitUnicodeString(&deviceName, FIRST_DEVICE_PATH);
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
	driverObject->MajorFunction[IRP_MJ_CLOSE] = my_close;
	driverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = my_internal_ioctl;

	deviceObject->Flags |= DO_BUFFERED_IO;
	deviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, FIRST_DOSDEVICE_PATH);
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
	RtlInitUnicodeString(&dosDeviceName, FIRST_DOSDEVICE_PATH);
	IoDeleteSymbolicLink(&dosDeviceName);

	IoDeleteDevice(driverObject->DeviceObject);
}

NTSTATUS STDCALL my_unsuported_function(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_NOT_SUPPORTED;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
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

NTSTATUS STDCALL my_close(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_close called\n");

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_internal_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_internal_ioctl called\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("No I/O stack location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	DbgPrint("IOCTL = 0x%08X\n", pIoStackIrp->Parameters.DeviceIoControl.IoControlCode);
	switch (pIoStackIrp->Parameters.DeviceIoControl.IoControlCode) {
		case MY_INTERNAL_IOCTL_HELLO:
			status = my_internal_ioctl_say_hello(Irp);
			break;

		default:
			status = STATUS_NOT_SUPPORTED;
			break;
	}

cleanup:
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS my_internal_ioctl_say_hello(PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_internal_ioctl_hello called\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("No I/O stack location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PCHAR msg = (PCHAR) Irp->AssociatedIrp.SystemBuffer;
	DbgPrint("Someone says: %s\n", msg);

	PCHAR outputBuffer = (PCHAR) Irp->AssociatedIrp.SystemBuffer;
	int outLength = pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength;
	RtlZeroMemory(outputBuffer, outLength);
	RtlCopyMemory(outputBuffer, "Hey! How are you?", outLength - 1);

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = outLength;

	return status;
}