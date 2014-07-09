#include <ntddk.h>
#include "public.h"
#include "driver.h"

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT  driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	DbgPrint("Load\n");

	driverObject->DriverUnload = my_unload;

	UNICODE_STRING deviceName;
	RtlInitUnicodeString(&deviceName, DEVICE_PATH);
	status = IoCreateDevice(driverObject,
							sizeof(device_context_t),
							&deviceName,
							FILE_DEVICE_UNKNOWN,
							FILE_DEVICE_SECURE_OPEN,
							FALSE,
							&deviceObject);
	if (status != STATUS_SUCCESS) {
		DbgPrint("Cannot create device\n");
		goto cleanup;
	}

	// Initialize the context
	device_context_t* context = (device_context_t*) deviceObject->DeviceExtension;
	KeInitializeMutex(&(context->mutex), 0);
	context->stack = NULL;

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		driverObject->MajorFunction[i] = my_unsuported_function;
	}
	driverObject->MajorFunction[IRP_MJ_CREATE] = my_create;
	driverObject->MajorFunction[IRP_MJ_CLOSE] = my_close;
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

NTSTATUS STDCALL my_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN mutexHeld = FALSE;

	DbgPrint("my_ioctl called\n");

	device_context_t* context = (device_context_t*) deviceObject->DeviceExtension;
	status = KeWaitForMutexObject(&(context->mutex), Executive, KernelMode, FALSE, NULL);
	if(status != STATUS_SUCCESS) {
		DbgPrint("Mutex problem\n");
		goto cleanup;
	}
	mutexHeld = TRUE;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("No I/O stack location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	DbgPrint("IOCTL = 0x%08X\n", pIoStackIrp->Parameters.DeviceIoControl.IoControlCode);
	switch (pIoStackIrp->Parameters.DeviceIoControl.IoControlCode) {
		case MY_IOCTL_PUSH:
			status = my_ioctl_push(context, Irp);
			break;

		case MY_IOCTL_POP:
			status = my_ioctl_pop(context, Irp);
			break;

		default:
			status = STATUS_NOT_SUPPORTED;
			break;
	}

cleanup:
	if (mutexHeld) {
		KeReleaseMutex(&(context->mutex), FALSE);
	}
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS my_ioctl_push(device_context_t* context, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_ioctl_push called\n");

	PCHAR c = (PCHAR) Irp->AssociatedIrp.SystemBuffer;

	my_stack_t* stack = (my_stack_t*) ExAllocatePoolWithTag(NonPagedPool, sizeof(my_stack_t), MY_POOL_TAG);
	if (!stack) {
		DbgPrint("Cannot allocate stack");
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanup;
	}

	stack->c = *c;
	stack->next = context->stack;
	context->stack = stack;

	DbgPrint("char received: %c\n", *c);

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;

	return status;
}

NTSTATUS my_ioctl_pop(device_context_t* context, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_ioctl_pop called\n");

	my_stack_t* stack = context->stack;
	if (stack) {
		PCHAR c = (PCHAR) Irp->AssociatedIrp.SystemBuffer;
		*c = stack->c;
		DbgPrint("char send: %c\n", *c);

		context->stack = stack->next;
		ExFreePoolWithTag(stack, MY_POOL_TAG);

		Irp->IoStatus.Information = 1;
	} else {
		status = STATUS_UNSUCCESSFUL;
		Irp->IoStatus.Information = 0;
	}

	Irp->IoStatus.Status = status;

	return status;
}