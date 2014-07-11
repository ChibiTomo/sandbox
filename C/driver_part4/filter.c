#include <ntddk.h>

#include "filter.h"
#include "public.h"

#define DbgPrint(format, ...) DbgPrint("[Filter] "format, ##__VA_ARGS__)

VOID STDCALL ExampleFilter_Unload(PDRIVER_OBJECT  DriverObject);
NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT  pDriverObject, PUNICODE_STRING  pRegistryPath);

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT  pDriverObject, PUNICODE_STRING  pRegistryPath) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("DriverEntry Called\n");

	PDEVICE_OBJECT pDeviceObject = NULL;
	NtStatus = IoCreateDevice(	pDriverObject,
								sizeof(EXAMPLE_FILTER_EXTENSION),
								NULL,
								FILE_DEVICE_UNKNOWN,
								FILE_DEVICE_SECURE_OPEN,
								FALSE,
								&pDeviceObject);

	if(NtStatus != STATUS_SUCCESS) {
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	for(int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		pDriverObject->MajorFunction[i] = ExampleFilter_UnSupportedFunction;
	}

	pDriverObject->MajorFunction[IRP_MJ_CLOSE]             = ExampleFilter_Close;
	pDriverObject->MajorFunction[IRP_MJ_CREATE]            = ExampleFilter_Create;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]    = ExampleFilter_IoControl;
	pDriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = ExampleFilter_IoControlInternal;
	pDriverObject->MajorFunction[IRP_MJ_READ]              = ExampleFilter_Read;
	pDriverObject->MajorFunction[IRP_MJ_WRITE]             = ExampleFilter_Write;

	pDriverObject->DriverUnload =  ExampleFilter_Unload;

	PEXAMPLE_FILTER_EXTENSION pExampleFilterDeviceContext = (PEXAMPLE_FILTER_EXTENSION) pDeviceObject->DeviceExtension;

	UNICODE_STRING usDeviceToFilter;
	RtlInitUnicodeString(&usDeviceToFilter, L"\\Device\\Example");
	NtStatus = IoAttachDevice(pDeviceObject, &usDeviceToFilter, &pExampleFilterDeviceContext->pNextDeviceInChain);

	if(NtStatus != STATUS_SUCCESS) {
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PDEVICE_OBJECT pFilteredDevice = pExampleFilterDeviceContext->pNextDeviceInChain;

	pDeviceObject->Flags |= pFilteredDevice->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO);
	pDeviceObject->DeviceType = pFilteredDevice->DeviceType;
	pDeviceObject->Characteristics = pFilteredDevice->Characteristics;
	pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

cleanup:
	if (NtStatus != STATUS_SUCCESS) {
		if (pDeviceObject) {
			IoDeleteDevice(pDeviceObject);
		}
	}
	return NtStatus;
}


/**********************************************************************
 *
 *  ExampleFilter_Unload
 *
 *    This is an optional unload function which is called when the
 *    driver is unloaded.
 *
 **********************************************************************/
VOID STDCALL ExampleFilter_Unload(PDRIVER_OBJECT  DriverObject) {
	PEXAMPLE_FILTER_EXTENSION pExampleFilterDeviceContext = (PEXAMPLE_FILTER_EXTENSION)DriverObject->DeviceObject->DeviceExtension;
	DbgPrint("ExampleFilter_Unload Called\n");

	IoDetachDevice(pExampleFilterDeviceContext->pNextDeviceInChain);
	IoDeleteDevice(DriverObject->DeviceObject);
}

/**********************************************************************
 * Internal Functions
 **********************************************************************/
NTSTATUS ExampleFilter_CompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);
NTSTATUS ExampleFilter_FixNullString(PCHAR pString, UINT uiSize);

/**********************************************************************
 *  ExampleFilter_Create
 *
 *    This is called when an instance of this driver is created (CreateFile)
 **********************************************************************/
NTSTATUS STDCALL ExampleFilter_Create(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("ExampleFilter_Create Called \r\n");

	IoSkipCurrentIrpStackLocation(Irp);

	PEXAMPLE_FILTER_EXTENSION pExampleFilterDeviceContext = (PEXAMPLE_FILTER_EXTENSION)DeviceObject->DeviceExtension;
	NtStatus = IoCallDriver(pExampleFilterDeviceContext->pNextDeviceInChain, Irp);

	DbgPrint("ExampleFilter_Create Exit 0x%0x \r\n", NtStatus);
	return NtStatus;
}


/**********************************************************************
 *  ExampleFilter_Close
 *
 *    This is called when an instance of this driver is closed (CloseHandle)
 **********************************************************************/
NTSTATUS STDCALL ExampleFilter_Close(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("ExampleFilter_Close Called \r\n");

	IoSkipCurrentIrpStackLocation(Irp);

	PEXAMPLE_FILTER_EXTENSION pExampleFilterDeviceContext = (PEXAMPLE_FILTER_EXTENSION)DeviceObject->DeviceExtension;
	NtStatus = IoCallDriver(pExampleFilterDeviceContext->pNextDeviceInChain, Irp);

	DbgPrint("ExampleFilter_Close Exit 0x%0x \r\n", NtStatus);
	return NtStatus;
}


/**********************************************************************
 *  ExampleFilter_IoControlInternal
 *
 *    These are IOCTL's which can only be sent by other drivers.
 **********************************************************************/
NTSTATUS STDCALL ExampleFilter_IoControlInternal(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_NOT_SUPPORTED;

	DbgPrint("ExampleFilter_IoControlInternal Called \r\n");

	IoSkipCurrentIrpStackLocation(Irp);

	PEXAMPLE_FILTER_EXTENSION pExampleFilterDeviceContext = (PEXAMPLE_FILTER_EXTENSION)DeviceObject->DeviceExtension;
	NtStatus = IoCallDriver(pExampleFilterDeviceContext->pNextDeviceInChain, Irp);

	DbgPrint("ExampleFilter_IoControlInternal Exit 0x%0x \r\n", NtStatus);

	return NtStatus;
}

/**********************************************************************
 *  ExampleFilter_IoControl
 *
 *    This is called when an IOCTL is issued on the device handle (DeviceIoControl)
 **********************************************************************/
NTSTATUS STDCALL ExampleFilter_IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_NOT_SUPPORTED;

	DbgPrint("ExampleFilter_IoControl Called \r\n");

	IoSkipCurrentIrpStackLocation(Irp);

	PEXAMPLE_FILTER_EXTENSION pExampleFilterDeviceContext = (PEXAMPLE_FILTER_EXTENSION)DeviceObject->DeviceExtension;
	NtStatus = IoCallDriver(pExampleFilterDeviceContext->pNextDeviceInChain, Irp);

	DbgPrint("ExampleFilter_IoControl Exit 0x%0x \r\n", NtStatus);

	return NtStatus;
}

/**********************************************************************
 *  ExampleFilter_Write
 *
 *    This is called when a write is issued on the device handle (WriteFile/WriteFileEx)
 **********************************************************************/
NTSTATUS STDCALL ExampleFilter_Write(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;

	DbgPrint("ExampleFilter_Write Called \r\n");

	IoSkipCurrentIrpStackLocation(Irp);

	PEXAMPLE_FILTER_EXTENSION pExampleFilterDeviceContext = (PEXAMPLE_FILTER_EXTENSION)DeviceObject->DeviceExtension;
	NtStatus = IoCallDriver(pExampleFilterDeviceContext->pNextDeviceInChain, Irp);

	DbgPrint("ExampleFilter_Write Exit 0x%0x \r\n", NtStatus);

	return NtStatus;
}

/**********************************************************************
 *  ExampleFilter_Read
 *
 *    This is called when a read is issued on the device handle (ReadFile/ReadFileEx)
 **********************************************************************/
NTSTATUS STDCALL ExampleFilter_Read(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("ExampleFilter_Read Called \r\n");

	PEXAMPLE_FILTER_EXTENSION pExampleFilterDeviceContext = (PEXAMPLE_FILTER_EXTENSION)DeviceObject->DeviceExtension;
	PCHAR pReadDataBuffer;
	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DbgPrint("No Irp location\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) ExampleFilter_CompletionRoutine, NULL, TRUE, TRUE, TRUE);

	NtStatus = IoCallDriver(pExampleFilterDeviceContext->pNextDeviceInChain, Irp);
	if(!NT_SUCCESS(NtStatus)) {
		DbgPrint("Cannot pass down Irp\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	if(!Irp->IoStatus.Information) {
		DbgPrint("No data read\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	if(DeviceObject->Flags & DO_BUFFERED_IO) {
		DbgPrint("ExampleFilter_Read - Use Buffered I/O\n");

		pReadDataBuffer = (PCHAR) Irp->AssociatedIrp.SystemBuffer;

		if(pReadDataBuffer && pIoStackIrp->Parameters.Read.Length > 0) {
			ExampleFilter_FixNullString(pReadDataBuffer, (UINT)Irp->IoStatus.Information);
		}
	} else if(DeviceObject->Flags & DO_DIRECT_IO) {
		DbgPrint("ExampleFilter_Read - Use Direct I/O\n");

		if(pIoStackIrp && Irp->MdlAddress) {
			pReadDataBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

			if(pReadDataBuffer && pIoStackIrp->Parameters.Read.Length) {
				ExampleFilter_FixNullString(pReadDataBuffer, (UINT)Irp->IoStatus.Information);
			}
		}
	} else {
		DbgPrint("ExampleFilter_Read - Use Neither I/O \r\n");
//		__try {
//			if(pIoStackIrp->Parameters.Read.Length > 0 && Irp->UserBuffer) {
//				ProbeForWrite(Irp->UserBuffer, pIoStackIrp->Parameters.Read.Length, TYPE_ALIGNMENT(char));
//				pReadDataBuffer = Irp->UserBuffer;
//
//				ExampleFilter_FixNullString(pReadDataBuffer, (UINT)Irp->IoStatus.Information);
//			}
//		} __except( EXCEPTION_EXECUTE_HANDLER ) {
//			NtStatus = GetExceptionCode();
//		}
	}

cleanup:
	Irp->IoStatus.Status = NtStatus;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DbgPrint("ExampleFilter_Read Exit 0x%0x \r\n", NtStatus);
	return NtStatus;
}

/**********************************************************************
 *  ExampleFilter_UnSupportedFunction
 *
 *    This is called when a major function is issued that isn't supported.
 **********************************************************************/
NTSTATUS STDCALL ExampleFilter_UnSupportedFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_NOT_SUPPORTED;

	DbgPrint("ExampleFilter_UnSupportedFunction Called \r\n");

	IoSkipCurrentIrpStackLocation(Irp);

	PEXAMPLE_FILTER_EXTENSION pExampleFilterDeviceContext = (PEXAMPLE_FILTER_EXTENSION)DeviceObject->DeviceExtension;
	NtStatus = IoCallDriver(pExampleFilterDeviceContext->pNextDeviceInChain, Irp);

	DbgPrint("ExampleFilter_UnSupportedFunction Exit 0x%0x \r\n", NtStatus);

	return NtStatus;
}

/**********************************************************************
 *  ExampleFilter_CompletionRoutine
 *
 *    This is called when an IRP has been completed (IoCompleteRequest)
 **********************************************************************/
NTSTATUS ExampleFilter_CompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context) {
	DbgPrint("ExampleFilter_CompletionRoutine Called\n");
	return STATUS_MORE_PROCESSING_REQUIRED;
}

/**********************************************************************
 *  ExampleFilter_FixNullString
 *
 *    This function will simply fix the NULL in the returned string.
 *    This is a very simple implementation for demonstration purposes.
 **********************************************************************/
NTSTATUS ExampleFilter_FixNullString(PCHAR pString, UINT uiSize) {
	NTSTATUS NtStatus = STATUS_SUCCESS;
	UINT uiIndex = 0;

	while(uiIndex < (uiSize - 1)) {
		if(pString[uiIndex] == 0) {
			pString[uiIndex] = ' ';
		}
		uiIndex++;
	}

	/*
	* Fix the end NULL
	*/
	pString[uiIndex] = 0;

	return NtStatus;
}

