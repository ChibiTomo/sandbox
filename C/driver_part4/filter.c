#include <ntddk.h>

#include "filter.h"
#include "public.h"

#define DbgPrint(format, ...) DbgPrint("[Filter] "format, ##__VA_ARGS__)

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS;

	driverObject->DriverUnload = my_unload;

	DbgPrint("Loaded.\n");

	PDEVICE_OBJECT deviceObject = NULL;
	status = IoCreateDevice(driverObject,
							sizeof(filter_device_extension_t),
							NULL,
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
	driverObject->MajorFunction[IRP_MJ_CLOSE] = ignored_function;
	driverObject->MajorFunction[IRP_MJ_CREATE] = ignored_function;
	driverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = ignored_function;

	filter_device_extension_t* context = (filter_device_extension_t*) deviceObject->DeviceExtension;

	UNICODE_STRING filteredDeviceName;
	RtlInitUnicodeString(&filteredDeviceName, DEVICE_PATH);
	status = IoAttachDevice(deviceObject, &filteredDeviceName, &context->nextDeviceInChain);

	if(status != STATUS_SUCCESS) {
		DbgPrint("Cannot attach to the device: %wZ\n", &filteredDeviceName);
		IoDeleteDevice(deviceObject);
		goto cleanup;
	}
	DbgPrint("Attached to: %wZ\n", &filteredDeviceName);

	PDEVICE_OBJECT filteredDevice = NULL;
	filteredDevice = context->nextDeviceInChain;

	deviceObject->Flags |= filteredDevice->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO);
	deviceObject->DeviceType = filteredDevice->DeviceType;
	deviceObject->Characteristics = filteredDevice->Characteristics;
	deviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

cleanup:
	return status;
}

VOID STDCALL my_unload(PDRIVER_OBJECT DriverObject) {
	DbgPrint("GoodBye!!\n");

	filter_device_extension_t* context = (filter_device_extension_t*) DriverObject->DeviceObject->DeviceExtension;
	if (context->nextDeviceInChain) {
		IoDetachDevice(context->nextDeviceInChain);
	}

	PDEVICE_OBJECT device = DriverObject->DeviceObject;
	if (device) {
		IoDeleteDevice(device);
	}
}

NTSTATUS STDCALL unsuported_function(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_NOT_SUPPORTED;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	DbgPrint("unsuported_function called: 0x%02X\n", pIoStackIrp->MajorFunction);

	return status;
}

NTSTATUS STDCALL ignored_function(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	DbgPrint("ignored_function called: 0x%02X\n", pIoStackIrp->MajorFunction);

	filter_device_extension_t* extension = (filter_device_extension_t*) DeviceObject->DeviceExtension;

	IoSkipCurrentIrpStackLocation(Irp);
	status = IoCallDriver(extension->nextDeviceInChain, Irp);

	return status;
}