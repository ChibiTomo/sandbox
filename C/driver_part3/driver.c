#include <ntddk.h>

#include "driver.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS; // default status
	PDEVICE_OBJECT deviceObject = NULL;

	driverObject->DriverUnload = my_unload;

	DbgPrint("RegistryPath=%wZ\n", registryPath);

	// Creation of the associated device
	UNICODE_STRING deviceName;
	RtlInitUnicodeString(&deviceName, L"\\Device\\YTDevice");
	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\YTDevice");

	status = IoCreateDevice(driverObject,
							sizeof(device_context_t),	// length of device extention (extra data to pass)
							&deviceName,
							FILE_DEVICE_UNKNOWN,
							FILE_DEVICE_SECURE_OPEN,
							FALSE,
							&deviceObject);

	if (status != STATUS_SUCCESS) {
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
	if (status != STATUS_SUCCESS) {
		if (deviceObject) {
			IoDeleteDevice(deviceObject);
		}
	}
	return status;
}

VOID STDCALL my_unload(PDRIVER_OBJECT DriverObject) {
	DbgPrint("Start to unload.\n");

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

	status = create_file_context((device_context_t*) DeviceObject->DeviceExtension, pIoStackIrp->FileObject);

cleanup:
	// The Irp must be completed to not be passed to other drivers in the stack.
	// The last driver to process the Irp must call IoCompleteRequest
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

	status = release_file_context((device_context_t*) DeviceObject->DeviceExtension, pIoStackIrp->FileObject);

cleanup:
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_write(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	UINT dwDataWritten = 0;

	DbgPrint("my_write called\n");
	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("no Irp pointer\n");
		goto cleanup;
	}

	PCHAR pWriteDataBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;
	if(!pWriteDataBuffer) {
		DbgPrint("no system buffer\n");
		goto cleanup;
	}

	status = my_write_data((file_info_t*) pIoStackIrp->FileObject->FsContext,
				pWriteDataBuffer, pIoStackIrp->Parameters.Write.Length, &dwDataWritten);
	if(status != STATUS_SUCCESS) {
		DbgPrint("my_write_data failed.\n");
		goto cleanup;
	}

	DbgPrint("written: %.*s\n", dwDataWritten, pWriteDataBuffer);

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = dwDataWritten;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_read(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_BUFFER_TOO_SMALL;
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
		status = STATUS_SUCCESS;
	}

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = dwDataRead;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS STDCALL create_file_context(device_context_t* device_context, PFILE_OBJECT pFileObject) {
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	file_info_t* file_info = NULL;
	BOOLEAN bNeedsToCreate = FALSE;
	BOOLEAN mutexHeld = FALSE;

	status = KeWaitForMutexObject(&device_context->mutex, Executive, KernelMode, FALSE, NULL);
	/*
	 * NT_SUCCESS(status) return TRUE for:
	 *	STATUS_SUCCESS
	 *	STATUS_ALERTED	(mutex NOT held!!!)
	 *	STATUS_USER_APC	(mutex NOT held!!!)
	 *	STATUS_TIMEOUT	(mutex NOT held!!!)
	 */
	if(status != STATUS_SUCCESS) {
		DbgPrint("Mutex not held...");
		goto cleanup;
	}
	mutexHeld = TRUE;
	file_info = device_context->info_list;
	bNeedsToCreate = TRUE;

	while(file_info && bNeedsToCreate) {
		if(RtlCompareUnicodeString(&file_info->fname, &pFileObject->FileName, TRUE) == 0) {
			bNeedsToCreate = FALSE;
			file_info->ref_count++;
			pFileObject->FsContext = (PVOID)file_info;

			status = STATUS_SUCCESS;
		} else {
			file_info = file_info->next;
		}
	}

	if(!bNeedsToCreate) {
		goto cleanup;
	}
	file_info = (file_info_t*)ExAllocatePoolWithTag(NonPagedPool, sizeof(file_info_t), POOL_TAG);

	if(!file_info) {
		status = STATUS_INSUFFICIENT_RESOURCES;
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

	status = STATUS_SUCCESS;

cleanup:
	if (mutexHeld) {
		DbgPrint("Release create context device_context->mutex\n");
		KeReleaseMutex(&device_context->mutex, FALSE);
	}
	return status;
}

NTSTATUS STDCALL release_file_context(device_context_t* device_context, PFILE_OBJECT pFileObject) {
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	file_info_t* file_info = NULL;
	file_info_t* irp_file_info = (file_info_t*) pFileObject->FsContext;
	BOOLEAN bNotFound = TRUE;
	BOOLEAN mutexHeld = FALSE;

	status = KeWaitForMutexObject(&device_context->mutex, Executive, KernelMode, FALSE, NULL);
	/*
	 * NT_SUCCESS(status) return TRUE for:
	 *	STATUS_SUCCESS
	 *	STATUS_ALERTED	(mutex NOT held!!!)
	 *	STATUS_USER_APC	(mutex NOT held!!!)
	 *	STATUS_TIMEOUT	(mutex NOT held!!!)
	 */
	if(status != STATUS_SUCCESS) {
		DbgPrint("Mutex not held...");
		goto cleanup;
	}
	mutexHeld = TRUE;

	file_info = device_context->info_list;

	if(!irp_file_info) {
		DbgPrint("No irp_file_info...");
		goto cleanup;
	}
	if(file_info == irp_file_info) {
		bNotFound = FALSE;
		status = STATUS_SUCCESS;
		irp_file_info->ref_count--;

		if(irp_file_info->ref_count == 0) {
			device_context->info_list = file_info->next;
			ExFreePool(irp_file_info);
		}
	} else {
		while(file_info && bNotFound == TRUE) {
			if(irp_file_info == file_info->next) {
				bNotFound = FALSE;
				status = STATUS_SUCCESS;
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

	if(bNotFound) {
		status = STATUS_UNSUCCESSFUL; /* Should Never Reach Here!!!!! */
	}
cleanup:
	if (mutexHeld) {
		DbgPrint("Release release context device_context->mutex\n");
		KeReleaseMutex(&device_context->mutex, FALSE);
	}
	return status;
}

BOOLEAN STDCALL my_read_data(file_info_t* file_info, PCHAR data, UINT length, UINT *str_length) {
	BOOLEAN bDataRead = FALSE;
	NTSTATUS status;
	*str_length = 0;
	BOOLEAN mutexHeld = FALSE;

	status = KeWaitForMutexObject(&file_info->mutex, Executive, KernelMode, FALSE, NULL);
	if(!NT_SUCCESS(status)) {
		goto cleanup;
	}
	mutexHeld = TRUE;

	DbgPrint("Before: Start Index = %i Stop Index = %i Size Of Buffer = %i\n",
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

cleanup:
	DbgPrint("After: Start Index = %i Stop Index = %i Size Of Buffer = %i\n",
					file_info->start_index,
					file_info->end_index,
					sizeof(file_info->circular_buffer));
	if (mutexHeld) {
		DbgPrint("Release read file_info->mutex\n");
		KeReleaseMutex(&file_info->mutex, FALSE);
	}
	return bDataRead;
}

NTSTATUS STDCALL my_write_data(file_info_t* file_info, PCHAR data, UINT length, UINT *str_length) {
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN mutexHeld = FALSE;

	*str_length = 0;

	status = KeWaitForMutexObject(&file_info->mutex, Executive, KernelMode, FALSE, NULL);
	/*
	 * NT_SUCCESS(status) return TRUE for:
	 *	STATUS_SUCCESS
	 *	STATUS_ALERTED	(mutex NOT held!!!)
	 *	STATUS_USER_APC	(mutex NOT held!!!)
	 *	STATUS_TIMEOUT	(mutex NOT held!!!)
	 */
	if (status != STATUS_SUCCESS) {
		DbgPrint("Problem with file_info mutex...\n");
		goto cleanup;
	}
	mutexHeld = TRUE;
	DbgPrint("Before: Start Index = %d Stop Index = %d Size Of Buffer = %d\n",
					file_info->start_index,
					file_info->end_index,
					sizeof(file_info->circular_buffer));

	if(file_info->start_index > (file_info->end_index + 1)) {
		/*
		 * To copy the buffer created in user mode to the kernel buffer we need to determine
		 * where the circular buffer pointers are.
		 *
		 *  [*****              ************************]
		 *     Stop           Start
		 *
		 * If the Start index is farther than the Stop index then we can assume that memory from the Start to the
		 * index (including wrapping).  So Start - (Stop + 1) = Valid Length.  We want to add 1 to the Stop index
		 * so that we do not get Start == Stop since this is reserved for empty buffer.
		 *
		 */
		UINT uiCopyLength = MIN((file_info->start_index - (file_info->end_index + 1)), length);

		DbgPrint("uiCopyLength = %i [ = MIN(%i, %i)]\n",
						uiCopyLength,
						(file_info->start_index - (file_info->end_index + 1)),
						length);

		if (uiCopyLength == 0) {
			DbgPrint("Nothing to copy.\n");
			goto cleanup;
		}
		RtlCopyMemory(file_info->circular_buffer + file_info->end_index, data, uiCopyLength);

		file_info->end_index += uiCopyLength;
		*str_length = uiCopyLength;

		goto cleanup;
	}

	/*
	 * Buffer:
	 *  [    ****************               ]
	 *     Start           Stop
	**/

	if (file_info->start_index == file_info->end_index + 1) {
		DbgPrint("Buffer full...\n");
		goto cleanup;
	}

	UINT uiLinearLengthAvailable;

	if (file_info->start_index == 0) {
		uiLinearLengthAvailable = sizeof(file_info->circular_buffer) - (file_info->end_index + 1);
	} else {
		uiLinearLengthAvailable = sizeof(file_info->circular_buffer) - file_info->end_index;
	}

	UINT uiCopyLength = MIN(uiLinearLengthAvailable, length);
	DbgPrint("uiCopyLength %d = MIN(uiLinearLengthAvailable %d, uiLength %d)\n",
					uiCopyLength, uiLinearLengthAvailable, length);
	if (uiCopyLength == 0) {
		DbgPrint("Nothing to copy\n");
		goto cleanup;
	}

	RtlCopyMemory(file_info->circular_buffer + file_info->end_index, data, uiCopyLength);

	file_info->end_index += uiCopyLength;
	*str_length = uiCopyLength;

	if (file_info->end_index < sizeof(file_info->circular_buffer)) {
		DbgPrint("Not the end of the buffer\n");
		goto cleanup;
	}

	file_info->end_index = 0;
	DbgPrint("file_info->end_index = 0 %i - %i = %i\n", length , uiCopyLength, (length - uiCopyLength));

	UINT uiSecondCopyLength = MIN(file_info->start_index - 1, length - uiCopyLength);
	DbgPrint("uiSecondCopyLength = %d\n", uiSecondCopyLength);
	if (uiSecondCopyLength == 0) {
		DbgPrint("Nothing to write anymore.\n");
		goto cleanup;
	}

	RtlCopyMemory(file_info->circular_buffer, data, uiSecondCopyLength);

	file_info->end_index = uiSecondCopyLength;
	*str_length = uiCopyLength + uiSecondCopyLength;

cleanup:
	DbgPrint("After: Start Index = %d Stop Index = %d Size Of Buffer = %d\n",
					file_info->start_index,
					file_info->end_index,
					sizeof(file_info->circular_buffer));
	if (mutexHeld) {
		DbgPrint("Release write file_info->mutex\n");
		KeReleaseMutex(&file_info->mutex, FALSE);
	}
	return status;
}