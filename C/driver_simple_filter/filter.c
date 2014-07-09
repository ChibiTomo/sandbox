#include <ntddk.h>
#include "public.h"
#include "filter.h"

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT  driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	DbgPrint("Load\n");

	driverObject->DriverUnload = my_unload;

	status = IoCreateDevice(driverObject,
							sizeof(device_extension_t),
							NULL, // Filter driver: device has no name
							FILE_DEVICE_UNKNOWN,
							FILE_DEVICE_SECURE_OPEN,
							FALSE,
							&deviceObject);
	if (status != STATUS_SUCCESS) {
		DbgPrint("Cannot create device\n");
		goto cleanup;
	}

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		// Filter driver: ignore function instead of unsupported
		driverObject->MajorFunction[i] = my_ignored_function;
	}
	driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = my_ioctl;

	device_extension_t* device_extension = (device_extension_t*) deviceObject->DeviceExtension;
	UNICODE_STRING deviceToFilter;
	RtlInitUnicodeString(&deviceToFilter, DEVICE_PATH);
	// Attach the filter device in the device stack
	status = IoAttachDevice(deviceObject, &deviceToFilter, &device_extension->next);
	if (status != STATUS_SUCCESS) {
		DbgPrint("Cannot attach device\n");
		goto cleanup;
	}

	PDEVICE_OBJECT filteredDevice = device_extension->next;

	// These flags are used only for ReadFile and WriteFile.
	deviceObject->Flags |= filteredDevice->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO);
	deviceObject->DeviceType = filteredDevice->DeviceType;
	deviceObject->Characteristics = filteredDevice->Characteristics;
	deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

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

	// Filter driver: detach device
	device_extension_t* device_extension = (device_extension_t*)driverObject->DeviceObject->DeviceExtension;
	IoDetachDevice(device_extension->next);

	// No symbolic link to remove

	IoDeleteDevice(driverObject->DeviceObject);
}

NTSTATUS STDCALL my_ignored_function(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("Ignored function called without Irp location\n");
		goto cleanup;
	}
	DbgPrint("Ignored function: 0x%02X\n", pIoStackIrp->MajorFunction);

cleanup:
	IoSkipCurrentIrpStackLocation(Irp);

	device_extension_t* device_extension = (device_extension_t*)deviceObject->DeviceExtension;
	status = IoCallDriver(device_extension->next, Irp);

	return status;
}

NTSTATUS STDCALL my_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN doCompleteIrp = TRUE;

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
			status = my_ioctl_push(deviceObject, Irp);
			if (status == STATUS_SUCCESS) {
				// push is asynchrone so we do not complete the Irp
				// (The filtered will do it for us)
				doCompleteIrp = FALSE;
			}
			break;

		case MY_IOCTL_POP:
			status = my_ioctl_pop(deviceObject, Irp);
			// Pop is synchrone so we complete the Irp
			break;

		default:
			my_ignored_function(deviceObject, Irp);
			doCompleteIrp = FALSE;
	}

cleanup:
	if (doCompleteIrp) {
		Irp->IoStatus.Status = status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return status;
}

NTSTATUS my_ioctl_push(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_ioctl_push called\n");

	PCHAR c = (PCHAR) Irp->AssociatedIrp.SystemBuffer;

	char old = *c;
	if (*c >= 97 && *c <= 97 + 26) {
		*c -= 32;
		DbgPrint("Translate: %d=>%c\n", old, *c);
	} else {
		DbgPrint("Error: input is not lower case: %c\n", *c);
		status = STATUS_UNSUCCESSFUL;
		Irp->IoStatus.Information = 0;
		goto cleanup;
	}

	IoSkipCurrentIrpStackLocation(Irp);
	device_extension_t* device_extension = (device_extension_t*) deviceObject->DeviceExtension;
	status = IoCallDriver(device_extension->next, Irp);

cleanup:
	return status;
}

NTSTATUS my_ioctl_pop(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_ioctl_pop called\n");

	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) my_completion_routine, NULL, TRUE, TRUE, TRUE);

	device_extension_t* device_extension = (device_extension_t*) deviceObject->DeviceExtension;
	status = IoCallDriver(device_extension->next, Irp);
	if (status != STATUS_SUCCESS) {
		DbgPrint("Error: filtered driver send back an error: 0x%08X\n", status);
		goto cleanup;
	}

	// Remaining actions after the filtered drivers
	PCHAR c = (PCHAR) Irp->AssociatedIrp.SystemBuffer;
	char old = *c;
	if (*c >= 65 && *c <= 65 + 26) {
		*c += 32;
		DbgPrint("Translate: %d=>%c\n", old, *c);
		Irp->IoStatus.Information = 1;
	} else {
		status = STATUS_UNSUCCESSFUL;
		DbgPrint("Error: input is not upper case: %c\n", *c);
	}

cleanup:
	return status;
}

NTSTATUS my_completion_routine(PDEVICE_OBJECT deviceObject, PIRP Irp, PVOID extension) {
	DbgPrint("my_completion_routine called\n");

	// This does not complete the Irp so it is possible to continue to use it
	// after the filtered drivers.
	return STATUS_MORE_PROCESSING_REQUIRED;
}