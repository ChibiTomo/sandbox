#include "driver.h"

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	DEBUG("Load\n");

	driverObject->DriverUnload = my_unload;

	UNICODE_STRING deviceName;
	RtlInitUnicodeString(&deviceName, DEVICE_PATH);

	status = IoCreateDevice(driverObject,
							sizeof(extension_t),
							&deviceName,
							FILE_DEVICE_UNKNOWN,
							FILE_DEVICE_SECURE_OPEN,
							FALSE,
							&deviceObject);
	if (status != STATUS_SUCCESS) {
		DEBUG("Cannot create device\n");
		goto cleanup;
	}

	extension_t* extension = (extension_t*) deviceObject->DeviceExtension;

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		driverObject->MajorFunction[i] = my_ignored_function;
	}
	driverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = my_internal_ioctl;

	UNICODE_STRING filteredDeviceName;
	RtlInitUnicodeString(&filteredDeviceName , L"\\Device\\Tcp");

	status = IoAttachDevice(deviceObject, &filteredDeviceName, &extension->topOfDeviceStack);
	if (status != STATUS_SUCCESS) {
		DEBUG("Cannot attach to %wZ\n", &filteredDeviceName);
		goto cleanup;
	}
	DEBUG("Successfully attached to %wZ\n", &filteredDeviceName);

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
	DEBUG("Unload\n");

	extension_t* extension = (extension_t*) driverObject->DeviceObject->DeviceExtension;
	IoDetachDevice(extension->topOfDeviceStack);

	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, DOSDEVICE_PATH);
	IoDeleteSymbolicLink(&dosDeviceName);

	IoDeleteDevice(driverObject->DeviceObject);
}

NTSTATUS STDCALL my_ignored_function(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_NOT_SUPPORTED;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DEBUG("Ignored function called without Irp location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	DEBUG("Ignored function: 0x%02X\n", pIoStackIrp->MajorFunction);

cleanup:
	IoSkipCurrentIrpStackLocation(Irp);

	extension_t* extension = (extension_t*) deviceObject->DeviceExtension;
	status = IoCallDriver (extension->topOfDeviceStack, Irp);

	return status;
}

NTSTATUS STDCALL my_internal_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("my_internal_ioctl\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DEBUG("No Irp location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	switch (pIoStackIrp->MinorFunction) {
		case TDI_CONNECT:
			DEBUG("TDI_CONNECT via TCP\n");
			break;

		case TDI_SEND:
			DEBUG("TDI_SEND packet TCP, size: %d\n", pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength);
			break;

		default:
			DEBUG("TDI function called: 0x%08X\n", pIoStackIrp->MinorFunction);
	}

cleanup:
	IoSkipCurrentIrpStackLocation(Irp);

	extension_t* extension = (extension_t*) deviceObject->DeviceExtension;
	status = IoCallDriver(extension->topOfDeviceStack, Irp );
	return status;
}