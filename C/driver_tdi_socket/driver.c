#include "driver.h"

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	DEBUG("Load\n");

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
		DEBUG("Cannot create device\n");
		goto cleanup;
	}

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		driverObject->MajorFunction[i] = my_unsupported_function;
	}
	driverObject->MajorFunction[IRP_MJ_CREATE] = my_create;
	driverObject->MajorFunction[IRP_MJ_CLOSE] = my_close;
	driverObject->MajorFunction[IRP_MJ_READ] = my_read;
	driverObject->MajorFunction[IRP_MJ_WRITE] = my_write;
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
	DEBUG("Unload\n");

	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, DOSDEVICE_PATH);
	IoDeleteSymbolicLink(&dosDeviceName);

	IoDeleteDevice(driverObject->DeviceObject);
}

NTSTATUS STDCALL my_unsupported_function(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_NOT_SUPPORTED;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DEBUG("Unsupported function called without Irp location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	DEBUG("Unsupported function: 0x%02X\n", pIoStackIrp->MajorFunction);

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

// This function should create a context for the FILE_OBJECT
// No connection for instance
NTSTATUS STDCALL my_create(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("my_create\n");
	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DEBUG("No Irp location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	// First: create the context
	file_extension_t* tdi_context =
			(file_extension_t*) ExAllocatePoolWithTag(NonPagedPool, sizeof(file_extension_t), MY_POOL_TAG);
	if (!tdi_context) {
		DEBUG("Cannot allocate tdi context");
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanup;
	}
	DEBUG("Context allocated: 0x%08X\n", tdi_context);

	// Second: initialize transport and connection
	status = tdi_initialize_handle(&(tdi_context->handle), pIoStackIrp);
	if (status != STATUS_SUCCESS) {
		DEBUG("Cannot initialize tdi handle\n");
		goto cleanup;
	}

	// Finally: associate context to file
	pIoStackIrp->FileObject->FsContext = (PVOID) tdi_context;

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_close(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("my_close\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DEBUG("No Irp location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	file_extension_t* tdi_context = (file_extension_t*) pIoStackIrp->FileObject->FsContext;
	DEBUG("Context retrieved: 0x%08X\n", tdi_context);
	if (!tdi_context) {
		DEBUG("No context\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	DEBUG("File handle: 0x%08X\n", tdi_context->handle);
	tdi_free_handle(&tdi_context->handle);
	if (status != STATUS_SUCCESS) {
		DEBUG("Cannot close file: 0x%08X\n", status);
		goto cleanup;
	}
	ExFreePoolWithTag(tdi_context, MY_POOL_TAG);
	pIoStackIrp->FileObject->FsContext = NULL;

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_read(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("my_read\n");

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_write(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("my_write\n");

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN completeIrp = TRUE;
	BOOLEAN fillIrp = TRUE;

	DEBUG("my_ioctl\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DEBUG("No Irp location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	file_extension_t* tdi_context = (file_extension_t*) pIoStackIrp->FileObject->FsContext;
	if (!tdi_context) {
		DEBUG("No context\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	switch (pIoStackIrp->Parameters.DeviceIoControl.IoControlCode) {
		case MY_IOCTL_TDI_CONNECT:
			status = my_ioctl_tdi_connect(tdi_context, Irp);
			fillIrp = FALSE;
			break;

		default:
			DEBUG("IOCTL: 0x%08X\n", pIoStackIrp->Parameters.DeviceIoControl.IoControlCode);
	}

cleanup:
	if (fillIrp) {
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;
	}
	if (completeIrp) {
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return status;
}

NTSTATUS my_ioctl_tdi_connect(file_extension_t* context, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	int success = 1;

	DEBUG("About to connect\n");

//	network_address_t* address = (network_address_t*) Irp->AssociatedIrp.SystemBuffer;

//	PFILE_OBJECT file_object = context->handle->connection;
//	PDEVICE_OBJECT tdi_device = IoGetRelatedDeviceObject(file_object);

	status = STATUS_UNSUCCESSFUL;

//cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = success;
	return status;
}