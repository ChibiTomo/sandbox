#include <ntddk.h>

#include "driver.h"
#include "public.h"

#define DbgPrint(format, ...) DbgPrint("[Driver] "format, ##__VA_ARGS__)

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("RegistryPath=%wZ\n", registryPath);

	driverObject->DriverUnload = my_unload;

	UNICODE_STRING deviceName;
	RtlInitUnicodeString(&deviceName, DEVICE_PATH);
	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, DOSDEVICE_PATH);

	PDEVICE_OBJECT deviceObject = NULL;
	status = IoCreateDevice(driverObject,
							sizeof(device_extension_t),
							&deviceName,
							FILE_DEVICE_UNKNOWN,
							FILE_DEVICE_SECURE_OPEN,
							FALSE,
							&deviceObject);

	if (status != STATUS_SUCCESS) {
		DbgPrint("Error while creating device: %wZ\n", &deviceName);
		goto cleanup;
	}

	device_extension_t* context = (device_extension_t*) deviceObject->DeviceExtension;
	KeInitializeMutex(&context->mutex, 0);
	context->extensions_list = NULL;

	for (UINT i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		driverObject->MajorFunction[i] = unsuported_function;
	}
	driverObject->MajorFunction[IRP_MJ_CREATE] = my_create;
	driverObject->MajorFunction[IRP_MJ_CLOSE] = my_close;

	deviceObject->Flags |= DO_BUFFERED_IO;
	deviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

	IoCreateSymbolicLink(&dosDeviceName, &deviceName);

cleanup:
	if (status != STATUS_SUCCESS) {
		if (deviceObject) {
			IoDeleteDevice(deviceObject);
		}
	}
	return status;
}

VOID STDCALL my_unload(PDRIVER_OBJECT DriverObject) {
	DbgPrint("Start to unload.\n");

	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, DOSDEVICE_PATH);
	IoDeleteSymbolicLink(&dosDeviceName);

	PDEVICE_OBJECT device = DriverObject->DeviceObject;
	if (device != NULL) {
		IoDeleteDevice(device);
	}
}

NTSTATUS STDCALL unsuported_function(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_NOT_SUPPORTED;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DbgPrint("unsuported_function called with invalid Irp.\n");
		goto cleanup;
	}
	DbgPrint("unsuported_function called: 0x%02X\n", pIoStackIrp->MajorFunction);
cleanup:
	return status;
}

NTSTATUS STDCALL my_create(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_create called\n");
	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DbgPrint("no Irp pointer\n");
		goto cleanup;
	}

	status = create_pipe_extension((device_extension_t*) DeviceObject->DeviceExtension, pIoStackIrp->FileObject);

cleanup:
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_close(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_close called\n");
	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DbgPrint("no Irp pointer\n");
		goto cleanup;
	}

	status = release_pipe_extension((device_extension_t*) DeviceObject->DeviceExtension, pIoStackIrp->FileObject);

cleanup:
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS create_pipe_extension(device_extension_t* deviceContext, PFILE_OBJECT pFileObject) {
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN mutexHeld = FALSE;
	DbgPrint("create_pipe_extension called\n");

	status = KeWaitForMutexObject(&deviceContext->mutex, Executive, KernelMode, FALSE, NULL);
	mutexHeld = TRUE;

cleanup:
	if (mutexHeld) {
		KeReleaseMutex(&deviceContext->mutex, FALSE);
	}
	return status;
}

NTSTATUS release_pipe_extension(device_extension_t* deviceContext, PFILE_OBJECT pFileObject) {
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN mutexHeld = FALSE;
	DbgPrint("close_pipe_extension called\n");

	status = KeWaitForMutexObject(&deviceContext->mutex, Executive, KernelMode, FALSE, NULL);
	mutexHeld = TRUE;

cleanup:
	if (mutexHeld) {
		KeReleaseMutex(&deviceContext->mutex, FALSE);
	}
	return status;
}