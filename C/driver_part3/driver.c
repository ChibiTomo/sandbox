#include <ntddk.h>

#include "driver.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS; // default status

	driverObject->DriverUnload = my_unload;

	DbgPrint("Hello!!\n");
	DbgPrint("RegistryPath=%wZ\n", registryPath);

	// Creation of the associated device
	PDEVICE_OBJECT deviceObject = NULL;
	UNICODE_STRING deviceName;
	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&deviceName, L"\\Device\\YTDevice");
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\YTDevice");

	status = IoCreateDevice(driverObject,
							sizeof(device_context_t),	// length of device extention (extra data to pass)
							&deviceName,
							FILE_DEVICE_UNKNOWN,
							FILE_DEVICE_SECURE_OPEN,
							FALSE,
							&deviceObject);

	if (status != STATUS_SUCCESS) {
		IoDeleteDevice(deviceObject);
		goto cleanup;
	}

	device_context_t* context = (device_context_t*) deviceObject->DeviceExtension;
	KeInitializeMutex(&context->mutex, 0);
	context->info_list = NULL;

	for (UINT i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		driverObject->MajorFunction[i] = unsuported_function;
	}
	driverObject->MajorFunction[IRP_MJ_CREATE] = my_create;
	driverObject->MajorFunction[IRP_MJ_CLOSE] = my_close;
	driverObject->MajorFunction[IRP_MJ_READ] = my_read;
	driverObject->MajorFunction[IRP_MJ_WRITE] = my_write;

	deviceObject->Flags |= DO_BUFFERED_IO;
	deviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

	IoCreateSymbolicLink(&dosDeviceName, &deviceName);

cleanup:
	return status;
}

VOID STDCALL my_unload(PDRIVER_OBJECT DriverObject) {
	DbgPrint("GoodBye!!\n");

	// Remove SymLinks
	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\YTDevice");
	IoDeleteSymbolicLink(&dosDeviceName);

	// Remove device
	PDEVICE_OBJECT device = DriverObject->DeviceObject;
	if (device != NULL) {
		IoDeleteDevice(device);
	}
}

NTSTATUS STDCALL unsuported_function(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_NOT_SUPPORTED;
	DbgPrint("unsuported_function called \n");
	return status;
}

NTSTATUS STDCALL my_create(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("my_create called \n");
	PIO_STACK_LOCATION pIoStackIrp = NULL;

	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		goto cleanup;
	}

	status = create_file_context((device_context_t*) DeviceObject->DeviceExtension, pIoStackIrp->FileObject);

cleanup:
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_close(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("my_close called \n");
	PIO_STACK_LOCATION pIoStackIrp = NULL;

	DbgPrint("About to IoGetCurrentIrpStackLocation\n");
	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	DbgPrint("IoGetCurrentIrpStackLocation done\n");
	if (!pIoStackIrp) {
		DbgPrint("no Irp pointer\n");
		goto cleanup;
	}

	DbgPrint("About to realease context\n");
	status = release_file_context((device_context_t*) DeviceObject->DeviceExtension, pIoStackIrp->FileObject);
	DbgPrint("Context released\n");

cleanup:
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_write(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("my_write called\n");

	PIO_STACK_LOCATION pIoStackIrp = NULL;
	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	PCHAR pWriteDataBuffer;

	if(!pIoStackIrp) {
		goto cleanup;
	}

	pWriteDataBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;

	if(!pWriteDataBuffer) {
		goto cleanup;
	}

	//DbgPrint(pWriteDataBuffer);

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = strlen(pWriteDataBuffer);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_read(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_BUFFER_TOO_SMALL;
	PIO_STACK_LOCATION pIoStackIrp = NULL;
	UINT dwDataRead = 0;
	PCHAR pReadDataBuffer;

	DbgPrint("my_read called \r\n");

	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);

	if(!pIoStackIrp) {
		goto cleanup;
	}
	pReadDataBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;

	if(!pReadDataBuffer || pIoStackIrp->Parameters.Read.Length <= 0) {
		goto cleanup;
	}

	if(my_read_data((file_info_t*) pIoStackIrp->FileObject->FsContext, pReadDataBuffer, pIoStackIrp->Parameters.Read.Length, &dwDataRead)) {
		NtStatus = STATUS_SUCCESS;
	}

cleanup:
	Irp->IoStatus.Status = NtStatus;
	Irp->IoStatus.Information = dwDataRead;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return NtStatus;
}

NTSTATUS STDCALL create_file_context(device_context_t* device_context, PFILE_OBJECT pFileObject) {
	NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
	file_info_t* file_info = NULL;
	BOOLEAN bNeedsToCreate = FALSE;

	NtStatus = KeWaitForMutexObject(&device_context->mutex, Executive, KernelMode, FALSE, NULL);

	if(!NT_SUCCESS(NtStatus)) {
		goto cleanup;
	}
	file_info = device_context->info_list;
	bNeedsToCreate = TRUE;

	while(file_info && bNeedsToCreate) {
		if(RtlCompareUnicodeString(&file_info->fname, &pFileObject->FileName, TRUE) == 0) {
			bNeedsToCreate = FALSE;
			file_info->ref_count++;
			pFileObject->FsContext = (PVOID)file_info;

			NtStatus = STATUS_SUCCESS;
		} else {
			file_info = file_info->next;
		}
	}

	if(!bNeedsToCreate) {
		goto cleanup;
	}
	file_info = (file_info_t*)ExAllocatePoolWithTag(NonPagedPool, sizeof(file_info_t), POOL_TAG);

	if(!file_info) {
		NtStatus = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanup;
	}
	file_info->next = device_context->info_list;
	device_context->info_list = file_info;

	file_info->ref_count = 1;
	file_info->fname.Length = 0;
	file_info->fname.MaximumLength = sizeof(file_info->unicode_fname);
	file_info->fname.Buffer = file_info->unicode_fname;
	file_info->start_index = 0;
	file_info->end_index = 0;

	KeInitializeMutex(&file_info->mutex, 0);

	RtlCopyUnicodeString(&file_info->fname, &pFileObject->FileName);

	pFileObject->FsContext = (PVOID)file_info;

	NtStatus = STATUS_SUCCESS;
	KeReleaseMutex(&device_context->mutex, FALSE);

cleanup:
	return NtStatus;
}

NTSTATUS STDCALL release_file_context(device_context_t* device_context, PFILE_OBJECT pFileObject) {
	NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
	file_info_t* file_info = NULL;
	file_info_t* irp_file_info = (file_info_t*) pFileObject->FsContext;
	BOOLEAN bNotFound = TRUE;
	DbgPrint("release file context\n");

	NtStatus = KeWaitForMutexObject(&device_context->mutex, Executive, KernelMode, FALSE, NULL);
	DbgPrint("KeWaitForMutexObject finished\n");

	if(!NT_SUCCESS(NtStatus)) {
		goto cleanup;
	}
	DbgPrint("mutex hold\n");
	file_info = device_context->info_list;

	if(!irp_file_info) {
		goto cleanup;
	}
	if(file_info == irp_file_info) {
		bNotFound = FALSE;
		NtStatus = STATUS_SUCCESS;
		irp_file_info->ref_count--;

		if(irp_file_info->ref_count == 0) {
			device_context->info_list = file_info->next;
			ExFreePool(irp_file_info);
		}
	} else {
		DbgPrint("looking for correct context\n");
		while(file_info && bNotFound == TRUE) {
			if(irp_file_info == file_info->next) {
				bNotFound = FALSE;
				NtStatus = STATUS_SUCCESS;
				irp_file_info->ref_count--;

				if(irp_file_info->ref_count == 0) {
					file_info->next = irp_file_info->next;
					ExFreePool(irp_file_info);
				}
			} else {
				file_info = file_info->next;
			}
		}
	}

	DbgPrint("bNotFound? %d\n", bNotFound);
	if(bNotFound) {
		NtStatus = STATUS_UNSUCCESSFUL; /* Should Never Reach Here!!!!! */
	}

	KeReleaseMutex(&device_context->mutex, FALSE);
	DbgPrint("KeReleaseMutex finished\n");
cleanup:
	DbgPrint("Success=%d\n", STATUS_SUCCESS);
	DbgPrint("status=%d\n", NtStatus);
	return NtStatus;
}

BOOLEAN STDCALL my_read_data(file_info_t* file_info, PCHAR data, UINT length, UINT *str_length) {
	BOOLEAN bDataRead = FALSE;
	NTSTATUS NtStatus;
	*str_length = 0;

	NtStatus = KeWaitForMutexObject(&file_info->mutex, Executive, KernelMode, FALSE, NULL);

	if(!NT_SUCCESS(NtStatus)) {
		goto cleanup;
	}

	DbgPrint("Start Index = %i Stop Index = %i Size Of Buffer = %i\n",
				file_info->start_index,
				file_info->end_index,
				sizeof(file_info->circular_buffer));

	/*
	 * Buffer:
	 *  [    ****************               ]
	 *     Start           Stop
	**/
	if(file_info->start_index < file_info->end_index) {
		UINT uiCopyLength = MIN(file_info->end_index - file_info->start_index, length);

		if(uiCopyLength) {
			RtlCopyMemory(data, file_info->circular_buffer + file_info->start_index, uiCopyLength);
			file_info->start_index += uiCopyLength;
			*str_length = uiCopyLength;
			bDataRead = TRUE;
		}
	} else {
		/*
		 * Buffer:
		 *  [****               **************]
		 *     Stop           Start
		**/

		if(file_info->start_index <= file_info->end_index) {
			goto cleanup;
		}
		UINT uiLinearLengthAvailable = sizeof(file_info->circular_buffer) - file_info->start_index;
		UINT uiCopyLength = MIN(uiLinearLengthAvailable, length);

		if(!uiCopyLength) {
			goto cleanup;
		}
		RtlCopyMemory(data, file_info->circular_buffer + file_info->start_index, uiCopyLength);

		file_info->start_index += uiCopyLength;
		*str_length =  uiCopyLength;
		bDataRead = TRUE;

		if(file_info->start_index < sizeof(file_info->circular_buffer)) {
			goto cleanup;
		}
		file_info->start_index = 0;

		if(length < uiCopyLength) {
			goto cleanup;
		}
		UINT uiSecondCopyLength = MIN(file_info->end_index - file_info->start_index, length - uiCopyLength);

		if(uiSecondCopyLength) {
			RtlCopyMemory(data, file_info->circular_buffer + file_info->start_index, uiCopyLength);
			file_info->start_index += uiSecondCopyLength;
			*str_length =  uiCopyLength + uiSecondCopyLength;
			bDataRead = TRUE;
		}
	}

	DbgPrint("Start Index = %i Stop Index = %i Size Of Buffer = %i\n",
					file_info->start_index,
					file_info->end_index,
					sizeof(file_info->circular_buffer));

	KeReleaseMutex(&file_info->mutex, FALSE);

cleanup:
	return bDataRead;
}

BOOLEAN STDCALL my_write_data(file_info_t* file_info, PCHAR data, UINT length, UINT *str_length) {
	BOOLEAN bDataWritten = FALSE;
	NTSTATUS NtStatus;
	*str_length = 0;

	NtStatus = KeWaitForMutexObject(&file_info->mutex, Executive, KernelMode, FALSE, NULL);

	if(!NT_SUCCESS(NtStatus)) {
		goto cleanup;
	}
	DbgPrint("Start Index = %i Stop Index = %i Size Of Buffer = %i\n",
					file_info->start_index,
					file_info->end_index,
					sizeof(file_info->circular_buffer));
	DbgPrint("Example_WriteData = %i > %i\n", file_info->start_index, (file_info->end_index + 1));
	if(file_info->start_index > (file_info->end_index + 1)) {
		UINT uiCopyLength = MIN((file_info->start_index - (file_info->end_index + 1)), length);

		DbgPrint("uiCopyLength = %i (%i, %i)\n",
						uiCopyLength,
						(file_info->start_index - (file_info->end_index + 1)),
						length);

		if(uiCopyLength) {
			RtlCopyMemory(file_info->circular_buffer + file_info->end_index, data, uiCopyLength);

			file_info->end_index += uiCopyLength;
			*str_length =  uiCopyLength;

			bDataWritten = TRUE;
		}
	} else {
		UINT uiLinearLengthAvailable;
		UINT uiCopyLength;

		if(file_info->start_index > file_info->end_index) {
			goto cleanup;
		}

		if(file_info->start_index == 0) {
			uiLinearLengthAvailable = sizeof(file_info->circular_buffer) - (file_info->end_index + 1);
		} else {
			uiLinearLengthAvailable = sizeof(file_info->circular_buffer) - file_info->end_index;
		}

		uiCopyLength = MIN(uiLinearLengthAvailable, length);

		DbgPrint("uiCopyLength %i = MIN(uiLinearLengthAvailable %i, uiLength %i)\n",
						uiCopyLength, uiLinearLengthAvailable, length);

		if(!uiCopyLength) {
			goto cleanup;
		}
		RtlCopyMemory(file_info->circular_buffer + file_info->end_index, data, uiCopyLength);

		file_info->end_index += uiCopyLength;
		*str_length =  uiCopyLength;

		bDataWritten = TRUE;

		if(file_info->end_index != sizeof(file_info->circular_buffer)) {
			goto cleanup;
		}
		file_info->end_index = 0;

		DbgPrint("file_info->end_index = 0 %i - %i = %i\n", length , uiCopyLength, (length - uiCopyLength));

		if(length < uiCopyLength) {
			goto cleanup;
		}
		UINT uiSecondCopyLength = MIN(file_info->start_index - (file_info->end_index + 1),
						length - uiCopyLength);

		DbgPrint("uiSecondCopyLength = 0 %i\n", uiSecondCopyLength);

		if(uiSecondCopyLength) {
			RtlCopyMemory(file_info->circular_buffer + file_info->end_index, data, uiSecondCopyLength);

			file_info->end_index += uiSecondCopyLength;
			*str_length =  uiCopyLength + uiSecondCopyLength;

			bDataWritten = TRUE;
		}
	}

	DbgPrint("Start Index = %i Stop Index = %i Size Of Buffer = %i\n",
					file_info->start_index,
					file_info->end_index,
					sizeof(file_info->circular_buffer));

	KeReleaseMutex(&file_info->mutex, FALSE);

cleanup:
	return bDataWritten;
}