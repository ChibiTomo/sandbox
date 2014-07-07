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
	driverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = my_internal_ioctl;

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
		status = STATUS_UNSUCCESSFUL;
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
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	status = release_pipe_extension((device_extension_t*) DeviceObject->DeviceExtension, pIoStackIrp->FileObject);

cleanup:
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS create_pipe_extension(device_extension_t* deviceExtentsion, PFILE_OBJECT pFileObject) {
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN mutexHeld = FALSE;
	DbgPrint("create_pipe_extension called\n");

	status = KeWaitForMutexObject(&deviceExtentsion->mutex, Executive, KernelMode, FALSE, NULL);
	if (status != STATUS_SUCCESS) {
		DbgPrint("Cannot hold mutex\n");
		goto cleanup;
	}
	mutexHeld = TRUE;

	file_extension_t* file_extension = deviceExtentsion->extensions_list;

	BOOLEAN needsToCreate = TRUE;
	while(file_extension && needsToCreate) {
		if (RtlCompareUnicodeString(&file_extension->fname, &pFileObject->FileName, TRUE) == 0) {
			needsToCreate = FALSE;
			file_extension->ref_count++;
			pFileObject->FsContext = (PVOID) file_extension;
		} else {
			file_extension = file_extension->next;
		}
	}

	if (!needsToCreate) {
		DbgPrint("File found: %wZ\n", &pFileObject->FileName);
		goto cleanup;
	}
	DbgPrint("Creating file: %wZ\n", &pFileObject->FileName);

	PIRP myIrp = IoAllocateIrp(pFileObject->DeviceObject->StackSize, FALSE);
	if (!myIrp) {
		DbgPrint("Cannot allocate Irp...\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

cleanup:
	if (mutexHeld) {
		KeReleaseMutex(&deviceExtentsion->mutex, FALSE);
	}
	return status;
}

NTSTATUS release_pipe_extension(device_extension_t* deviceExtentsion, PFILE_OBJECT pFileObject) {
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN mutexHeld = FALSE;
	DbgPrint("close_pipe_extension called\n");

	status = KeWaitForMutexObject(&deviceExtentsion->mutex, Executive, KernelMode, FALSE, NULL);
	if (status != STATUS_SUCCESS) {
		DbgPrint("Cannot hold mutex\n");
		goto cleanup;
	}
	mutexHeld = TRUE;

cleanup:
	if (mutexHeld) {
		KeReleaseMutex(&deviceExtentsion->mutex, FALSE);
	}
	return status;
}

NTSTATUS STDCALL my_internal_ioctl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	UINT dwDataWritten = 0;

	DbgPrint("my_internal_ioctl called \r\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DbgPrint("No Irp location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	switch (pIoStackIrp->Parameters.DeviceIoControl.IoControlCode) {
		case MY_IOCTL_INTERNAL_CREATE_EXTENSION:
			status = create_extension(Irp, pIoStackIrp, &dwDataWritten);
			break;

		case MY_IOCTL_INTERNAL_CLOSE_EXTENSION:
			//status = close_extension(Irp, pIoStackIrp, &dwDataWritten);
			break;

		default:
			status = STATUS_NOT_SUPPORTED;
			break;
	}
cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = dwDataWritten;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS create_extension(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten) {
	NTSTATUS status = STATUS_SUCCESS;

	DbgPrint("create_extension called\n");

	PFILE_OBJECT pFileObject = (PFILE_OBJECT) Irp->AssociatedIrp.SystemBuffer;
	device_extension_t* device_extension = (device_extension_t*) pFileObject->DeviceObject->DeviceExtension;

	file_extension_t* file_extension =
		(file_extension_t*) ExAllocatePoolWithTag(NonPagedPool, sizeof(file_extension_t), POOL_TAG);

	if(!file_extension) {
		DbgPrint("Cannot allocate file_extension\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	file_extension->next = device_extension->extensions_list;
	device_extension->extensions_list = file_extension;

	file_extension->ref_count = 1;
	file_extension->fname.Length = 0;
	file_extension->fname.MaximumLength = sizeof(file_extension->unicode_fname);
	file_extension->fname.Buffer = file_extension->unicode_fname;
	file_extension->start_index = 0;
	file_extension->end_index = 0;

	KeInitializeMutex(&file_extension->mutex, 0);

	RtlCopyUnicodeString(&file_extension->fname, &pFileObject->FileName);

	pFileObject->FsContext = (PVOID) file_extension;

cleanup:
	return status;
}