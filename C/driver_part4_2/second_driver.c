#include <ntddk.h>
#include "public.h"
#include "second_driver.h"

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT  driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	DbgPrint("Load\n");

	driverObject->DriverUnload = my_unload;

	UNICODE_STRING deviceName;
	RtlInitUnicodeString(&deviceName, SECOND_DEVICE_PATH);
	status = IoCreateDevice(driverObject,
							sizeof(device_extension_t),
							&deviceName,
							FILE_DEVICE_UNKNOWN,
							FILE_DEVICE_SECURE_OPEN,
							FALSE,
							&deviceObject);
	if (status != STATUS_SUCCESS) {
		DbgPrint("Cannot create device\n");
		goto cleanup;
	}
	device_extension_t* device_extension = (device_extension_t*) deviceObject->DeviceExtension;
	device_extension->next = NULL;

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		driverObject->MajorFunction[i] = my_unsuported_function;
	}
	driverObject->MajorFunction[IRP_MJ_CREATE] = my_create;
	driverObject->MajorFunction[IRP_MJ_CLOSE] = my_close;
	driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = my_ioctl;

	deviceObject->Flags |= DO_BUFFERED_IO;
	deviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, SECOND_DOSDEVICE_PATH);
	IoCreateSymbolicLink(&dosDeviceName, &deviceName);

	try_attach_to_other_driver(driverObject);

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
	RtlInitUnicodeString(&dosDeviceName, SECOND_DOSDEVICE_PATH);
	IoDeleteSymbolicLink(&dosDeviceName);

	// Get the second device
	PDEVICE_OBJECT device = driverObject->DeviceObject;

	while (device) {
		PDEVICE_OBJECT device_to_delete = device;
		// Get the first device
		device = device->NextDevice;

		// Check if the device is attached to an other one
		// and delete it if so
		device_extension_t* device_extension = (device_extension_t*) device_to_delete->DeviceExtension;
		DbgPrint("device_extension->next=0x%08X\n", device_extension->next);
		if (device_extension->next) {
			// This is the second device
			DbgPrint("Can detach device\n");
			IoDetachDevice(device_extension->next);
		}

		// Delete device
		IoDeleteDevice(device_to_delete);
	}
}

void try_attach_to_other_driver(PDRIVER_OBJECT driverObject) {
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	status = IoCreateDevice(driverObject,
							sizeof(device_extension_t),
							NULL,
							FILE_DEVICE_UNKNOWN,
							FILE_DEVICE_SECURE_OPEN,
							FALSE,
							&deviceObject);
	if (status != STATUS_SUCCESS) {
		DbgPrint("Cannot create filter device\n");
		goto cleanup;
	}

	device_extension_t* device_extension = (device_extension_t*) deviceObject->DeviceExtension;
	UNICODE_STRING deviceToFilter;
	RtlInitUnicodeString(&deviceToFilter, FIRST_DEVICE_PATH);

	// Attach the filter device in the device stack
	DbgPrint("About to attach device\n");
	status = IoAttachDevice(deviceObject, &deviceToFilter, &device_extension->next);
	DbgPrint("Attached device: 0x%08X\n", deviceObject->AttachedDevice);
	DbgPrint("device_extension->next=0x%08X\n", device_extension->next);

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

	DbgPrint("Filter device successfully created\n");
cleanup:
	if (status != STATUS_SUCCESS) {
		if (deviceObject) {
			IoDeleteDevice(deviceObject);
		}
	}
}

NTSTATUS STDCALL my_unsuported_function(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_NOT_SUPPORTED;

	DbgPrint("deviceObject: 0x%08X\n", deviceObject);

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
	DbgPrint("deviceObject: 0x%08X\n", deviceObject);

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_close(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_close called\n");
	DbgPrint("deviceObject: 0x%08X\n", deviceObject);

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_ioctl called\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("No I/O stack location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	DbgPrint("IOCTL = 0x%08X\n", pIoStackIrp->Parameters.DeviceIoControl.IoControlCode);
	switch (pIoStackIrp->Parameters.DeviceIoControl.IoControlCode) {
		case MY_IOCTL_SAY_HELLO:
			status = my_ioctl_say_hello(Irp, deviceObject);
			break;

		default:
			DbgPrint("Not supported\n");
			status = STATUS_NOT_SUPPORTED;
			break;
	}

cleanup:
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS my_ioctl_say_hello(PIRP Irp, PDEVICE_OBJECT deviceObject) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("my_ioctl_say_hello called\n");
	DbgPrint("deviceObject=0x%08X\n", deviceObject);

	PDRIVER_OBJECT driver = deviceObject->DriverObject;
	PDEVICE_OBJECT device = driver->DeviceObject;
	DbgPrint("driver->DeviceObject=0x%08X\n", driver->DeviceObject);

	while (device) {
		PDEVICE_OBJECT currentDevice = device;
		DbgPrint("currentDevice=0x%08X\n", currentDevice);
		device = device->NextDevice;
		DbgPrint("NextDevice=0x%08X\n", device);

		device_extension_t* device_extension = (device_extension_t*) currentDevice->DeviceExtension;
		DbgPrint("device_extension->next=0x%08X\n", device_extension->next);
		if (device_extension->next) {
			status = say_hello(device_extension->next);
			if (status != STATUS_SUCCESS) {
				DbgPrint("0x%08X does not answer correctly\n", device_extension->next);
				goto cleanup;
			}
		}
	}

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;

	return status;
}

NTSTATUS say_hello(PDEVICE_OBJECT deviceObject) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("Say Hello to 0x%08X\n", deviceObject);

	PIRP myIrp = IoAllocateIrp(deviceObject->StackSize, FALSE);
	if (!myIrp) {
		DbgPrint("Cannot allocate Irp\n");
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanup;
	}

	PIO_STACK_LOCATION pMyIoStackLocation = IoGetNextIrpStackLocation(myIrp);
	if (!pMyIoStackLocation) {
		DbgPrint("No stack location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	pMyIoStackLocation->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pMyIoStackLocation->Parameters.DeviceIoControl.IoControlCode = MY_INTERNAL_IOCTL_HELLO;

	char buffer[BUFFER_SIZE] = "Hello you!";

	pMyIoStackLocation->Parameters.DeviceIoControl.InputBufferLength  = BUFFER_SIZE;
	pMyIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength = BUFFER_SIZE;

	myIrp->AssociatedIrp.SystemBuffer = buffer;
	myIrp->MdlAddress = NULL;

	IoSetCompletionRoutine(myIrp, my_completion_routine, NULL, TRUE, TRUE, TRUE);

	status = IoCallDriver(deviceObject, myIrp);
	if (status != STATUS_SUCCESS) {
		DbgPrint("Error while calling the other driver\n");
		goto cleanup;
	}
	DbgPrint("The other one says: %s\n", myIrp->AssociatedIrp.SystemBuffer);

cleanup:
	if (myIrp) {
		IoFreeIrp(myIrp);
	}
	return status;
}

NTSTATUS STDCALL my_completion_routine(PDEVICE_OBJECT deviceObject, PIRP Irp, PVOID Context) {
	DbgPrint("my_completion_routine\n");

	return STATUS_MORE_PROCESSING_REQUIRED;
}