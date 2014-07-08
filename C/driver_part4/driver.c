#include <ntddk.h>
#include "driver.h"
#include "public.h"

#define DbgPrint(format, ...) DbgPrint("[Driver] "format, ##__VA_ARGS__)

VOID STDCALL Example_Unload(PDRIVER_OBJECT  DriverObject);
NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT  pDriverObject, PUNICODE_STRING  pRegistryPath);

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT  pDriverObject, PUNICODE_STRING  pRegistryPath) {
	NTSTATUS NtStatus = STATUS_SUCCESS;
	PDEVICE_OBJECT pDeviceObject = NULL;

	pDriverObject->DriverUnload =  Example_Unload;

	DbgPrint("DriverEntry Called \r\n");

	UNICODE_STRING usDriverName;
	RtlInitUnicodeString(&usDriverName, L"\\Device\\Example");
	NtStatus = IoCreateDevice(pDriverObject,
								sizeof(EXAMPLE_DEVICE_CONTEXT),
								&usDriverName,
								FILE_DEVICE_UNKNOWN,
								FILE_DEVICE_SECURE_OPEN,
								FALSE,
								&pDeviceObject);

	if(NtStatus != STATUS_SUCCESS) {
		goto cleanup;
	}

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		pDriverObject->MajorFunction[i] = Example_UnSupportedFunction;
	}

	pDriverObject->MajorFunction[IRP_MJ_CLOSE]             = Example_Close;
	pDriverObject->MajorFunction[IRP_MJ_CREATE]            = Example_Create;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]    = Example_IoControl;
	pDriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = Example_IoControlInternal;
	pDriverObject->MajorFunction[IRP_MJ_READ]              = USE_READ_FUNCTION;
	pDriverObject->MajorFunction[IRP_MJ_WRITE]             = USE_WRITE_FUNCTION;

	PEXAMPLE_DEVICE_CONTEXT pExampleDeviceContext = (PEXAMPLE_DEVICE_CONTEXT)pDeviceObject->DeviceExtension;
	KeInitializeMutex(&pExampleDeviceContext->kListMutex, 0);
	pExampleDeviceContext->pExampleList  = NULL;

	pDeviceObject->Flags |= IO_TYPE;
	pDeviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

	UNICODE_STRING usDosDeviceName;
	RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\Example");
	IoCreateSymbolicLink(&usDosDeviceName, &usDriverName);

cleanup:
	if (NtStatus != STATUS_SUCCESS) {
		if (pDeviceObject) {
			IoDeleteDevice(pDeviceObject);
		}
	}
	return NtStatus;
}

VOID STDCALL Example_Unload(PDRIVER_OBJECT  DriverObject) {
	UNICODE_STRING usDosDeviceName;

	DbgPrint("Example_Unload Called\n");

	RtlInitUnicodeString(&usDosDeviceName, L"\\DosDevices\\Example");
	IoDeleteSymbolicLink(&usDosDeviceName);

	IoDeleteDevice(DriverObject->DeviceObject);
}

#define IOCTL_CREATE_NEW_RESOURCE_CONTEXT  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_CLOSE_RESOURCE_CONTEXT       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED,FILE_READ_DATA | FILE_WRITE_DATA)

// Called when any user do a CreateFile with \\.\Example
NTSTATUS STDCALL Example_Create(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("Example_Create Called\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DbgPrint("No Irp location\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	NtStatus = Example_CreatePipeContext((PEXAMPLE_DEVICE_CONTEXT) DeviceObject->DeviceExtension, pIoStackIrp->FileObject);

cleanup:
	Irp->IoStatus.Status = NtStatus;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	DbgPrint("Example_Create Exit 0x%08X\n", NtStatus);
	return NtStatus;
}

// Called each time user call CloseHandle
NTSTATUS STDCALL Example_Close(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("Example_Close Called\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DbgPrint("No Irp location\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	NtStatus = Example_ReleasePipeContext((PEXAMPLE_DEVICE_CONTEXT)DeviceObject->DeviceExtension, pIoStackIrp->FileObject);

cleanup:
	Irp->IoStatus.Status = NtStatus;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	DbgPrint("Example_Close Exit 0x%08X\n", NtStatus);
	return NtStatus;
}

// Associate an extension for to the given file
NTSTATUS STDCALL Example_CreatePipeContext(PEXAMPLE_DEVICE_CONTEXT pExampleDeviceContext, PFILE_OBJECT pFileObject) {
	NTSTATUS NtStatus = STATUS_SUCCESS;
	BOOLEAN mutexHeld = FALSE;

	NtStatus = KeWaitForMutexObject(&pExampleDeviceContext->kListMutex, Executive, KernelMode, FALSE, NULL);

	if(NtStatus != STATUS_SUCCESS) {
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	mutexHeld = TRUE;

	PEXAMPLE_LIST pExampleList = pExampleDeviceContext->pExampleList;

	BOOLEAN bNeedsToCreate = TRUE;
	while(pExampleList && bNeedsToCreate) {
		if(RtlCompareUnicodeString(&pExampleList->usPipeName, &pFileObject->FileName, TRUE) == 0) {
			bNeedsToCreate = FALSE;
			pExampleList->uiRefCount++;
			pFileObject->FsContext = (PVOID)pExampleList;
		} else {
			pExampleList = pExampleList->pNext;
		}
	}

	if(!bNeedsToCreate) {
		DbgPrint("File found: %wZ\n", &(pFileObject->FileName));
		goto cleanup;
	}
	DbgPrint("Creating file: %wZ\n", &(pFileObject->FileName));
	PIRP MyIrp = IoAllocateIrp(pFileObject->DeviceObject->StackSize, FALSE);

	if(!MyIrp) {
		DbgPrint("Cannot allocate Irp\n");
		NtStatus = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanup;
	}

	PIO_STACK_LOCATION pMyIoStackLocation = IoGetNextIrpStackLocation(MyIrp);

	pMyIoStackLocation->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pMyIoStackLocation->Parameters.DeviceIoControl.IoControlCode = IOCTL_CREATE_NEW_RESOURCE_CONTEXT;
	pMyIoStackLocation->Parameters.DeviceIoControl.InputBufferLength  = sizeof(FILE_OBJECT);
	pMyIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength = 0;

	MyIrp->AssociatedIrp.SystemBuffer = pFileObject;
	MyIrp->MdlAddress                 = NULL;

	IoSetCompletionRoutine(MyIrp, Example_SampleCompletionRoutine, NULL, TRUE, TRUE, TRUE);

	NtStatus = IoCallDriver(pFileObject->DeviceObject, MyIrp);

cleanup:
	if (mutexHeld) {
		KeReleaseMutex(&pExampleDeviceContext->kListMutex, FALSE);
	}
	DbgPrint("Example_CreatePipeContext Exit 0x%08X\n", NtStatus);
	return NtStatus;
}

// Realease the extension of the given file
NTSTATUS STDCALL Example_ReleasePipeContext(PEXAMPLE_DEVICE_CONTEXT pExampleDeviceContext, PFILE_OBJECT pFileObject) {
	NTSTATUS NtStatus = STATUS_SUCCESS;
	BOOLEAN mutexHeld = FALSE;

	BOOLEAN bNotFound = TRUE;
	PDEVICE_OBJECT pTopOfStackDevice = NULL;
	LARGE_INTEGER StartOffset;
	StartOffset.QuadPart = 0;
	IO_STATUS_BLOCK StatusBlock;
	memset(&StatusBlock, 0, sizeof(IO_STATUS_BLOCK));

	NtStatus = KeWaitForMutexObject(&pExampleDeviceContext->kListMutex, Executive, KernelMode, FALSE, NULL);
	if(NtStatus != STATUS_SUCCESS) {
		DbgPrint("Cannot hold mutex on device\n");
		goto cleanup;
	}
	mutexHeld = TRUE;

	PEXAMPLE_LIST pExampleListFromIrp = (PEXAMPLE_LIST)pFileObject->FsContext;
	if(!pExampleListFromIrp) {
		DbgPrint("No extension associated to the Irp\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PEXAMPLE_LIST pExampleList = pExampleDeviceContext->pExampleList;

	while(pExampleList && bNotFound == TRUE) {
		if(pExampleList == pExampleListFromIrp) {
			bNotFound = FALSE;
			pExampleListFromIrp->uiRefCount--;
			DbgPrint("File found: %wZ\n", &(pFileObject->FileName));
		} else {
			pExampleList = pExampleList->pNext;
		}
	}

	if(bNotFound) {
		DbgPrint("Cannot find the file: %wZ\n", &(pFileObject->FileName));
		NtStatus = STATUS_UNSUCCESSFUL; /* Should Never Reach Here!!!!! */
		goto cleanup;
	}

	if(pExampleListFromIrp->uiRefCount > 0) {
		DbgPrint("File still in use. Not deleted.\n");
		goto cleanup;
	}

	pExampleDeviceContext->pExampleList = pExampleList->pNext;

	pTopOfStackDevice = IoGetRelatedDeviceObject(pFileObject);
	PIRP MyIrp = IoBuildAsynchronousFsdRequest(IRP_MJ_INTERNAL_DEVICE_CONTROL,
												pTopOfStackDevice,
												NULL,
												0,
												&StartOffset,
												&StatusBlock);

	if(!MyIrp) {
		DbgPrint("Cannot allocate Irp\n");
		NtStatus = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanup;
	}

	PIO_STACK_LOCATION pMyIoStackLocation = IoGetNextIrpStackLocation(MyIrp);

	pMyIoStackLocation->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pMyIoStackLocation->Parameters.DeviceIoControl.IoControlCode = IOCTL_CLOSE_RESOURCE_CONTEXT;

	pMyIoStackLocation->Parameters.DeviceIoControl.InputBufferLength  = sizeof(EXAMPLE_LIST);
	pMyIoStackLocation->Parameters.DeviceIoControl.OutputBufferLength = 0;

	MyIrp->AssociatedIrp.SystemBuffer = pExampleListFromIrp;
	MyIrp->MdlAddress                 = NULL;

	IoSetCompletionRoutine(MyIrp, Example_SampleCompletionRoutine, NULL, TRUE, TRUE, TRUE);

	NtStatus = IoCallDriver(pTopOfStackDevice, MyIrp);

cleanup:
	if (mutexHeld) {
		KeReleaseMutex(&pExampleDeviceContext->kListMutex, FALSE);
	}
	DbgPrint("Example_ReleasePipeContext Exit 0x%08X\n", NtStatus);
	return NtStatus;
}

// Called only by drivers through IoCallDriver (and others), with MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL
NTSTATUS STDCALL Example_IoControlInternal(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_SUCCESS;
	UINT dwDataWritten = 0;

	DbgPrint("Example_IoControlInternal Called\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) { // Should Never Be NULL!
		DbgPrint("No Irp location");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	DbgPrint("Example_IoControlInternal Called IOCTL = 0x%08X\n",
			pIoStackIrp->Parameters.DeviceIoControl.IoControlCode);

	switch(pIoStackIrp->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_CREATE_NEW_RESOURCE_CONTEXT:
			NtStatus = Example_CreateNewResource(Irp, pIoStackIrp, &dwDataWritten);
			break;

		case IOCTL_CLOSE_RESOURCE_CONTEXT:
			NtStatus = Example_DestroyResource(Irp, pIoStackIrp, &dwDataWritten);
			break;

		default:
			NtStatus = STATUS_NOT_SUPPORTED;
			break;
	}

cleanup:
	Irp->IoStatus.Status = NtStatus;
	Irp->IoStatus.Information = dwDataWritten;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DbgPrint("Example_IoControlInternal Exit 0x%08X\n", NtStatus);
	return NtStatus;
}


// Called each time user call DeviceIoControl
NTSTATUS STDCALL Example_IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION pIoStackIrp = NULL;
	UINT dwDataWritten = 0;

	DbgPrint("Example_IoControl Called\n");

	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) { // Should Never Be NULL!
		DbgPrint("No Irp location\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	DbgPrint("Example_IoControl Called IOCTL = 0x%08X\n", pIoStackIrp->Parameters.DeviceIoControl.IoControlCode);

	switch(pIoStackIrp->Parameters.DeviceIoControl.IoControlCode) {
		case MY_IOCTL_DIRECT_IN_IO:
			NtStatus = Example_HandleSampleIoctl_DirectInIo(Irp, pIoStackIrp, &dwDataWritten);
			break;

		case MY_IOCTL_DIRECT_OUT_IO:
			NtStatus = Example_HandleSampleIoctl_DirectOutIo(Irp, pIoStackIrp, &dwDataWritten);
			break;

		case MY_IOCTL_BUFFERED_IO:
			NtStatus = Example_HandleSampleIoctl_BufferedIo(Irp, pIoStackIrp, &dwDataWritten);
			break;

//		case IOCTL_EXAMPLE_SAMPLE_NEITHER_IO:
//			NtStatus = Example_HandleSampleIoctl_NeitherIo(Irp, pIoStackIrp, &dwDataWritten);
//			break;

		default:
			NtStatus = STATUS_NOT_SUPPORTED;
			break;
	}

cleanup:
	Irp->IoStatus.Status = NtStatus;
	Irp->IoStatus.Information = dwDataWritten;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DbgPrint("Example_IoControl Exit 0x%08X\n", NtStatus);
	return NtStatus;
}

// Called when user calls WriteFile/WriteFileEx (direct mode)
NTSTATUS STDCALL Example_WriteDirectIO(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_SUCCESS;
	UINT dwDataWritten = 0;

	DbgPrint("Example_WriteDirectIO Called\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("No Irp location\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	if(!Irp->MdlAddress) {
		DbgPrint("No MdlAddress\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PCHAR pWriteDataBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if(!pWriteDataBuffer) {
		DbgPrint("No buffer found\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	if(!Example_WriteData((PEXAMPLE_LIST) pIoStackIrp->FileObject->FsContext,
			pWriteDataBuffer, pIoStackIrp->Parameters.Write.Length, &dwDataWritten))
	{
		DbgPrint("Cannot write data\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

cleanup:
	Irp->IoStatus.Status = NtStatus;
	Irp->IoStatus.Information = dwDataWritten;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DbgPrint("Example_WriteDirectIO Exit 0x%08X\n", NtStatus);
	return NtStatus;
}

// Called when user calls WriteFile/WriteFileEx (buffered mode)
NTSTATUS STDCALL Example_WriteBufferedIO(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_SUCCESS;
	UINT dwDataWritten = 0;

	DbgPrint("Example_WriteBufferedIO Called\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("No Irp location\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PCHAR pWriteDataBuffer = (PCHAR) Irp->AssociatedIrp.SystemBuffer;
	if(!pWriteDataBuffer) {
		DbgPrint("No buffer found\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	if(!Example_WriteData((PEXAMPLE_LIST) pIoStackIrp->FileObject->FsContext,
			pWriteDataBuffer, pIoStackIrp->Parameters.Write.Length, &dwDataWritten))
	{
		DbgPrint("Cannot write data\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

cleanup:
	Irp->IoStatus.Status = NtStatus;
	Irp->IoStatus.Information = dwDataWritten;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DbgPrint("Example_WriteBufferedIO Exit 0x%08X\n", NtStatus);
	return NtStatus;
}

// Called when user calls WriteFile/WriteFileEx (neither mode)
//NTSTATUS Example_WriteNeither(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
//    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
//    PIO_STACK_LOCATION pIoStackIrp = NULL;
//    PCHAR pWriteDataBuffer;
//    UINT dwDataWritten = 0;
//    DbgPrint("Example_WriteNeither Called\n");
//
//    /*
//     * Each time the IRP is passed down the driver stack a new stack location is added
//     * specifying certain parameters for the IRP to the driver.
//     */
//    pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
//
//    if(pIoStackIrp) {
//        /*
//         * We need this in an exception handler or else we could trap.
//         */
//        __try {
//                if(Irp->UserBuffer) {
//                    ProbeForRead(Irp->UserBuffer, pIoStackIrp->Parameters.Write.Length, TYPE_ALIGNMENT(char));
//                    pWriteDataBuffer = Irp->UserBuffer;
//
//                    if(Example_WriteData((PEXAMPLE_LIST)pIoStackIrp->FileObject->FsContext, pWriteDataBuffer, pIoStackIrp->Parameters.Write.Length, &dwDataWritten)) {
//                        NtStatus = STATUS_SUCCESS;
//                    }
//                }
//        } __except( EXCEPTION_EXECUTE_HANDLER ) {
//              NtStatus = GetExceptionCode();
//        }
//    }
//
//    Irp->IoStatus.Status = NtStatus;
//    Irp->IoStatus.Information = dwDataWritten;//
//    IoCompleteRequest(Irp, IO_NO_INCREMENT);
//
//    DbgPrint("Example_WriteNeither Exit 0x%08X\n", NtStatus);
//    return NtStatus;
//}


// Called when user calls ReadFile/ReadFileEx (direct mode)
NTSTATUS STDCALL Example_ReadDirectIO(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_SUCCESS;
	UINT dwDataRead = 0;

	DbgPrint("Example_ReadDirectIO Called\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("No Irp location\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	if(!Irp->MdlAddress) {
		DbgPrint("No MdlAddress\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PCHAR pReadDataBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if(!pReadDataBuffer) {
		DbgPrint("No buffer found\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	if(pIoStackIrp->Parameters.Read.Length <= 0) {
		DbgPrint("Nothing to read\n");
		goto cleanup;
	}

	if(!Example_ReadData((PEXAMPLE_LIST) pIoStackIrp->FileObject->FsContext,
			pReadDataBuffer, pIoStackIrp->Parameters.Read.Length, &dwDataRead))
	{
		DbgPrint("Cannot read data\n");
		NtStatus = STATUS_BUFFER_TOO_SMALL;
		goto cleanup;
	}

cleanup:
	Irp->IoStatus.Status = NtStatus;
	Irp->IoStatus.Information = dwDataRead;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DbgPrint("Example_ReadDirectIO Exit 0x%08X\n", NtStatus);
	return NtStatus;
}

// Called when user calls ReadFile/ReadFileEx (buffered mode)
NTSTATUS STDCALL Example_ReadBufferedIO(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_SUCCESS;
	UINT dwDataRead = 0;

	DbgPrint("Example_ReadBufferedIO Called\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("No Irp location\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PCHAR pReadDataBuffer = (PCHAR) Irp->AssociatedIrp.SystemBuffer;

	if(!pReadDataBuffer) {
		DbgPrint("No buffer found\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	if(pIoStackIrp->Parameters.Read.Length <= 0) {
		DbgPrint("Nothing to read\n");
		goto cleanup;
	}

	if(!Example_ReadData((PEXAMPLE_LIST)pIoStackIrp->FileObject->FsContext,
			pReadDataBuffer, pIoStackIrp->Parameters.Read.Length, &dwDataRead))
	{
		DbgPrint("Cannot read data\n");
		NtStatus = STATUS_BUFFER_TOO_SMALL;
		goto cleanup;
	}

cleanup:
	Irp->IoStatus.Status = NtStatus;
	Irp->IoStatus.Information = dwDataRead;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DbgPrint("Example_ReadBufferedIO Exit 0x%0x \r\n", NtStatus);
	return NtStatus;
}

// Called when user calls ReadFile/ReadFileEx (neither mode)
//NTSTATUS Example_ReadNeither(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
//    NTSTATUS NtStatus = STATUS_BUFFER_TOO_SMALL;
//    PIO_STACK_LOCATION pIoStackIrp = NULL;
//    UINT dwDataRead = 0;
//    PCHAR pReadDataBuffer;
//
//    DbgPrint("Example_ReadNeither Called\n");
//
//    /*
//     * Each time the IRP is passed down the driver stack a new stack location is added
//     * specifying certain parameters for the IRP to the driver.
//     */
//    pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
//
//    if(pIoStackIrp) {
//        /*
//         * We need this in an exception handler or else we could trap.
//         */
//        __try {
//            if(pIoStackIrp->Parameters.Read.Length > 0 && Irp->UserBuffer) {
//                ProbeForWrite(Irp->UserBuffer, pIoStackIrp->Parameters.Read.Length, TYPE_ALIGNMENT(char));
//                pReadDataBuffer = Irp->UserBuffer;
//
//                if(Example_ReadData((PEXAMPLE_LIST)pIoStackIrp->FileObject->FsContext,
//						pReadDataBuffer, pIoStackIrp->Parameters.Read.Length, &dwDataRead))
//                {
//                    NtStatus = STATUS_SUCCESS;
//                }
//            }
//        } __except( EXCEPTION_EXECUTE_HANDLER ) {
//              NtStatus = GetExceptionCode();
//        }
//    }
//
//    Irp->IoStatus.Status = NtStatus;
//    Irp->IoStatus.Information = dwDataRead;
//
//    IoCompleteRequest(Irp, IO_NO_INCREMENT);
//    DbgPrint("Example_ReadNeither Exit 0x%08X\n", NtStatus);
//    return NtStatus;
//}

// Called each time an unimplmented function is called
NTSTATUS STDCALL Example_UnSupportedFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_NOT_SUPPORTED;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if(!pIoStackIrp) {
		DbgPrint("Example_UnSupportedFunction Called without Irp location\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	DbgPrint("Example_UnSupportedFunction Called: 0x%02X\n", pIoStackIrp->MajorFunction);

cleanup:
	Irp->IoStatus.Status = NtStatus;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return NtStatus;
}

// Write data into the circular buffer of the given file
BOOLEAN STDCALL Example_WriteData(PEXAMPLE_LIST pExampleList, PCHAR pData, UINT uiLength, UINT *pdwStringLength) {
	BOOLEAN mutexHeld = FALSE;
	BOOLEAN bDataWritten = FALSE;

	*pdwStringLength = 0;

	NTSTATUS NtStatus = KeWaitForMutexObject(&pExampleList->kInstanceBufferMutex, Executive, KernelMode, FALSE, NULL);
	if (NtStatus != STATUS_SUCCESS) {
		DbgPrint("Cannot hold mutex on file\n");
		goto cleanup;
	}
	mutexHeld = TRUE;

	if((pExampleList->uiStartIndex == (pExampleList->uiStopIndex + 1))
		|| ((pExampleList->uiStopIndex == sizeof(pExampleList->pCircularBuffer) - 1)
			&& pExampleList->uiStartIndex == 0))
	{
		DbgPrint("Buffer full\n");
		goto cleanup;
	}

	/*
	*  [*****              ************************]
	*     Stop           Start
	*/
	if(pExampleList->uiStartIndex > (pExampleList->uiStopIndex + 1)) {
		UINT uiCopyLength = MIN((pExampleList->uiStartIndex - (pExampleList->uiStopIndex + 1)), uiLength);

		if(uiCopyLength <= 0) {
			DbgPrint("Nothing to write\n");
			goto cleanup;
		}
		RtlCopyMemory(pExampleList->pCircularBuffer + pExampleList->uiStopIndex, pData, uiCopyLength);

		pExampleList->uiStopIndex += uiCopyLength;
		*pdwStringLength = uiCopyLength;

		bDataWritten = TRUE;
	} else {
		/*
		*  [    ****************                     ]
		*     Start           Stop
		*/
		if(pExampleList->uiStartIndex <= pExampleList->uiStopIndex) {
			UINT uiLinearLengthAvailable = sizeof(pExampleList->pCircularBuffer) - pExampleList->uiStopIndex;
			if(pExampleList->uiStartIndex == 0) {
				uiLinearLengthAvailable = sizeof(pExampleList->pCircularBuffer) - (pExampleList->uiStopIndex + 1);
			}
			UINT uiCopyLength = MIN(uiLinearLengthAvailable, uiLength);

			if(uiCopyLength != 0) {
				RtlCopyMemory(pExampleList->pCircularBuffer + pExampleList->uiStopIndex, pData, uiCopyLength);

				pExampleList->uiStopIndex += uiCopyLength;
				*pdwStringLength =  uiCopyLength;

				bDataWritten = TRUE;
			}

			if(pExampleList->uiStopIndex != sizeof(pExampleList->pCircularBuffer)) {
				DbgPrint("End of buffer not reached: %d/%d\n", pExampleList->uiStopIndex, sizeof(pExampleList->pCircularBuffer));
				goto cleanup;
			}
			pExampleList->uiStopIndex = 0;

			if((uiLength - uiCopyLength) <= 0) {
				DbgPrint("No more to write\n");
				goto cleanup;
			}
			UINT uiSecondCopyLength = MIN(pExampleList->uiStartIndex - (pExampleList->uiStopIndex + 1),
											uiLength - uiCopyLength);

			if(uiSecondCopyLength <= 0) {
				DbgPrint("No more to write (ERROR)\n");
				goto cleanup;
			}
			RtlCopyMemory(pExampleList->pCircularBuffer + pExampleList->uiStopIndex, pData, uiSecondCopyLength);

			pExampleList->uiStopIndex += uiSecondCopyLength;
			*pdwStringLength =  uiCopyLength + uiSecondCopyLength;

			bDataWritten = TRUE;
		}
	}

cleanup:
	if (mutexHeld) {
		KeReleaseMutex(&pExampleList->kInstanceBufferMutex, FALSE);
	}
	DbgPrint("start=%d, end=%d, size=%d\n", pExampleList->uiStartIndex, pExampleList->uiStopIndex, sizeof(pExampleList->pCircularBuffer));
	return bDataWritten;
}

// Read data from the circular buffer of the given file
BOOLEAN STDCALL Example_ReadData(PEXAMPLE_LIST pExampleList, PCHAR pData, UINT uiLength, UINT *pdwStringLength) {
	BOOLEAN mutexHeld = FALSE;
	BOOLEAN bDataRead = FALSE;

	*pdwStringLength = 0;

	NTSTATUS NtStatus = KeWaitForMutexObject(&pExampleList->kInstanceBufferMutex, Executive, KernelMode, FALSE, NULL);
	if (NtStatus != STATUS_SUCCESS) {
		DbgPrint("Cannot hold mutex on file\n");
		goto cleanup;
	}
	mutexHeld = TRUE;

	if (pExampleList->uiStartIndex == pExampleList->uiStopIndex) {
		DbgPrint("Buffer is empty\n");
		goto cleanup;
	}

	/*
	*  [    ****************               ]
	*     Start           Stop
	*/
	if(pExampleList->uiStartIndex < pExampleList->uiStopIndex) {
		UINT uiCopyLength = MIN(pExampleList->uiStopIndex - pExampleList->uiStartIndex, uiLength);

		if(uiCopyLength == 0) {
			DbgPrint("Nothing to read\n");
			goto cleanup;
		}
		RtlCopyMemory(pData, pExampleList->pCircularBuffer + pExampleList->uiStartIndex, uiCopyLength);
		pExampleList->uiStartIndex += uiCopyLength;
		*pdwStringLength =  uiCopyLength;
		bDataRead = TRUE;
	} else {
		/*
		*  [****               **************]
		*     Stop           Start
		*/

		if(pExampleList->uiStartIndex <= pExampleList->uiStopIndex) {
			DbgPrint("Nothing to read or buffer empty (ERROR)\n");
			goto cleanup;
		}
		UINT uiLinearLengthAvailable = sizeof(pExampleList->pCircularBuffer) - pExampleList->uiStartIndex;
		UINT uiCopyLength = MIN(uiLinearLengthAvailable, uiLength);

		if(uiCopyLength > 0) {
			RtlCopyMemory(pData, pExampleList->pCircularBuffer + pExampleList->uiStartIndex, uiCopyLength);

			pExampleList->uiStartIndex += uiCopyLength;
			*pdwStringLength =  uiCopyLength;
			bDataRead = TRUE;
		}

		if(pExampleList->uiStartIndex != sizeof(pExampleList->pCircularBuffer)) {
			DbgPrint("End of buffer not reached: %d/%d\n", pExampleList->uiStartIndex, sizeof(pExampleList->pCircularBuffer));
			goto cleanup;
		}
		pExampleList->uiStartIndex = 0;

		if(uiLength - uiCopyLength <= 0) {
			DbgPrint("Nothing more to read\n");
			goto cleanup;
		}
		UINT uiSecondCopyLength = MIN(pExampleList->uiStopIndex - pExampleList->uiStartIndex, uiLength - uiCopyLength);

		if(uiSecondCopyLength) {
			RtlCopyMemory(pData, pExampleList->pCircularBuffer + pExampleList->uiStartIndex, uiCopyLength);
			pExampleList->uiStartIndex += uiSecondCopyLength;
			*pdwStringLength =  uiCopyLength + uiSecondCopyLength;
			bDataRead = TRUE;
		}
	}

cleanup:
	if (mutexHeld) {
		KeReleaseMutex(&pExampleList->kInstanceBufferMutex, FALSE);
	}
	DbgPrint("start=%d, end=%d, size=%d\n", pExampleList->uiStartIndex, pExampleList->uiStopIndex, sizeof(pExampleList->pCircularBuffer));
	return bDataRead;
}


// Is string zero terminated
BOOLEAN STDCALL Example_IsStringTerminated(PCHAR pString, UINT uiLength, UINT *pdwStringLength) {
	BOOLEAN bStringIsTerminated = FALSE;
	UINT uiIndex = 0;

	*pdwStringLength = 0;

	while(uiIndex < uiLength && bStringIsTerminated == FALSE) {
		if(pString[uiIndex] == '\0') {
			*pdwStringLength = uiIndex + 1; /* Include the total count we read, includes the NULL */
			bStringIsTerminated = TRUE;
		} else {
			uiIndex++;
		}
	}

	return bStringIsTerminated;
}


// IOCTL direct in
NTSTATUS STDCALL Example_HandleSampleIoctl_DirectInIo(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("Example_HandleSampleIoctl_DirectInIo Called\n");

	PCHAR pInputBuffer = Irp->AssociatedIrp.SystemBuffer;
	if (!pInputBuffer) {
		DbgPrint("No input buffer\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	if(!Irp->MdlAddress) {
		DbgPrint("No Mdl address\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PCHAR pOutputBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (!pOutputBuffer) {
		DbgPrint("No output buffer\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	UINT dwDataRead = 0;

	if(!Example_IsStringTerminated(pInputBuffer,
		pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength, &dwDataRead))
	{
		DbgPrint("Input string not null terminated\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	DbgPrint("UserModeMessage = '%s'", pInputBuffer);

	PCHAR pReturnData = "IOCTL - Direct In I/O From Kernel!";
	UINT dwDataSize = sizeof("IOCTL - Direct In I/O From Kernel!");
	DbgPrint("%i >= %i", pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength, dwDataSize);

	if(pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength < dwDataSize) {
		DbgPrint("Buffer too small\n");
		NtStatus = STATUS_BUFFER_TOO_SMALL;
		goto cleanup;
	}
	RtlCopyMemory(pOutputBuffer, pReturnData, dwDataSize);
	*pdwDataWritten = dwDataSize;

cleanup:
	return NtStatus;
}

// IOCTL direct out
NTSTATUS STDCALL Example_HandleSampleIoctl_DirectOutIo(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("Example_HandleSampleIoctl_DirectOutIo Called \r\n");

	PCHAR pInputBuffer = Irp->AssociatedIrp.SystemBuffer;
	if (!pInputBuffer) {
		DbgPrint("No input buffer\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	if(!Irp->MdlAddress) {
		DbgPrint("No Mdl address\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PCHAR pOutputBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (!pOutputBuffer) {
		DbgPrint("No output buffer\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	UINT dwDataRead = 0;
	if(!Example_IsStringTerminated(pInputBuffer,
		pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength, &dwDataRead))
	{
		DbgPrint("Input string not null terminated\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	DbgPrint("UserModeMessage = '%s'", pInputBuffer);

	PCHAR pReturnData = "IOCTL - Direct In I/O From Kernel!";
	UINT dwDataSize = sizeof("IOCTL - Direct In I/O From Kernel!");
	DbgPrint("%i >= %i", pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength, dwDataSize);

	if(pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength < dwDataSize) {
		DbgPrint("Buffer too small\n");
		NtStatus = STATUS_BUFFER_TOO_SMALL;
		goto cleanup;
	}
	RtlCopyMemory(pOutputBuffer, pReturnData, dwDataSize);
	*pdwDataWritten = dwDataSize;

cleanup:
	return NtStatus;
}


// IOCTL buffered
NTSTATUS STDCALL Example_HandleSampleIoctl_BufferedIo(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("Example_HandleSampleIoctl_BufferedIo Called\n");

	PCHAR pInputBuffer = Irp->AssociatedIrp.SystemBuffer;
	if (!pInputBuffer) {
		DbgPrint("No input buffer\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PCHAR pOutputBuffer = Irp->AssociatedIrp.SystemBuffer;
	if (!pOutputBuffer) {
		DbgPrint("No output buffer\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	UINT dwDataRead = 0;
	if(!Example_IsStringTerminated(pInputBuffer,
		pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength, &dwDataRead))
	{
		DbgPrint("Input string not null terminated\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	DbgPrint("UserModeMessage = '%s'", pInputBuffer);

	PCHAR pReturnData = "IOCTL - Buffered I/O From Kernel!";
	UINT dwDataSize = sizeof("IOCTL - Buffered I/O From Kernel!");
	DbgPrint("%i >= %i", pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength, dwDataSize);

	if(pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength < dwDataSize) {
		DbgPrint("Buffer too small\n");
		NtStatus = STATUS_BUFFER_TOO_SMALL;
		goto cleanup;
	}
	RtlCopyMemory(pOutputBuffer, pReturnData, dwDataSize);
	*pdwDataWritten = dwDataSize;

cleanup:
	return NtStatus;
}



// IOCTL neither
//NTSTATUS Example_HandleSampleIoctl_NeitherIo(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten)
//{
//    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
//    PCHAR pInputBuffer;
//    PCHAR pOutputBuffer;
//    UINT dwDataRead = 0, dwDataWritten = 0;
//    PCHAR pReturnData = "IOCTL - Neither I/O From Kernel!";
//    UINT dwDataSize = sizeof("IOCTL - Neither I/O From Kernel!");
//
//    DbgPrint("Example_HandleSampleIoctl_NeitherIo Called \r\n");
//
//    /*
//     * METHOD_NEITHER
//     *
//     *    Input Buffer = Parameters.DeviceIoControl.Type3InputBuffer
//     *    Ouput Buffer = Irp->UserBuffer
//     *
//     *    Input Size   =  Parameters.DeviceIoControl.InputBufferLength
//     *    Output Size  =  Parameters.DeviceIoControl.OutputBufferLength
//     *
//     */
//
//
//    pInputBuffer = pIoStackIrp->Parameters.DeviceIoControl.Type3InputBuffer;
//    pOutputBuffer = Irp->UserBuffer;
//
//    if(pInputBuffer && pOutputBuffer)
//    {
//
//        /*
//         * We need this in an exception handler or else we could trap.
//         */
//        __try {
//
//                ProbeForRead(pInputBuffer, pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength, TYPE_ALIGNMENT(char));
//
//                /*
//                 * We need to verify that the string is NULL terminated. Bad things can happen
//                 * if we access memory not valid while in the Kernel.
//                 */
//               if(Example_IsStringTerminated(pInputBuffer, pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength, &dwDataRead))
//               {
//                    DbgPrint("UserModeMessage = '%s'", pInputBuffer);
//
//                    ProbeForWrite(pOutputBuffer, pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength, TYPE_ALIGNMENT(char));
//                    DbgPrint("%i >= %i", pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength, dwDataSize);
//                    if(pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength >= dwDataSize)
//                    {
//                        /*
//                         * We use "RtlCopyMemory" in the kernel instead of memcpy.
//                         * RtlCopyMemory *IS* memcpy, however it's best to use the
//                         * wrapper in case this changes in the future.
//                         */
//                        RtlCopyMemory(pOutputBuffer, pReturnData, dwDataSize);
//                        *pdwDataWritten = dwDataSize;
//                        NtStatus = STATUS_SUCCESS;
//                    }
//                    else
//                    {
//                        *pdwDataWritten = dwDataSize;
//                        NtStatus = STATUS_BUFFER_TOO_SMALL;
//                    }
//
//               }
//
//
//        } __except( EXCEPTION_EXECUTE_HANDLER ) {
//
//              NtStatus = GetExceptionCode();
//        }
//
//    }
//
//
//    return NtStatus;
//}

/**********************************************************************
 *  Example_CreateNewResource
 *
 *    Sample Implementation of a Private IOCTL
 **********************************************************************/
NTSTATUS STDCALL Example_CreateNewResource(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("Example_CreateNewResource \n");

	PEXAMPLE_LIST pExampleList =
			(PEXAMPLE_LIST) ExAllocatePoolWithTag(NonPagedPool, sizeof(EXAMPLE_LIST), EXAMPLE_POOL_TAG);
	if(!pExampleList) {
		DbgPrint("Cannot allocate extension");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PFILE_OBJECT pFileObject = (PFILE_OBJECT)Irp->AssociatedIrp.SystemBuffer;
	PEXAMPLE_DEVICE_CONTEXT pExampleDeviceContext =
		(PEXAMPLE_DEVICE_CONTEXT)pFileObject->DeviceObject->DeviceExtension;

	pExampleList->pNext = pExampleDeviceContext->pExampleList;
	pExampleDeviceContext->pExampleList = pExampleList;

	pExampleList->uiRefCount = 1;
	pExampleList->usPipeName.Length = 0;
	pExampleList->usPipeName.MaximumLength = sizeof(pExampleList->szwUnicodeString);
	pExampleList->usPipeName.Buffer = pExampleList->szwUnicodeString;
	pExampleList->uiStartIndex = 0;
	pExampleList->uiStopIndex = 0;

	KeInitializeMutex(&pExampleList->kInstanceBufferMutex, 0);

	RtlCopyUnicodeString(&pExampleList->usPipeName, &pFileObject->FileName);

	pFileObject->FsContext = (PVOID)pExampleList;

cleanup:
	return NtStatus;
}

/**********************************************************************
 *  Example_DestroyResource
 *
 *    Sample Implementation of a Private IOCTL
 **********************************************************************/
NTSTATUS STDCALL Example_DestroyResource(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("Example_DestroyResource \n");

	PEXAMPLE_LIST pExampleList = Irp->AssociatedIrp.SystemBuffer;
	if(!pExampleList) {
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	ExFreePool(pExampleList);

cleanup:
	return NtStatus;
}


/**********************************************************************
 *  Example_SampleCompletionRoutine
 *
 *    Sample Implementation of a Completion Routine
 **********************************************************************/
NTSTATUS STDCALL Example_SampleCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID  Context) {
	DbgPrint("Example_SampleCompletionRoutine\n");

	IoFreeIrp(Irp);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

#ifndef STATUS_CONTINUE_COMPLETION

#define STATUS_CONTINUE_COMPLETION STATUS_SUCCESS

#endif

/**********************************************************************
 *  Example_SampleCompletionRoutineWithIoManager
 *
 *    Sample Implementation of a Completion Routine
 **********************************************************************/
NTSTATUS STDCALL Example_SampleCompletionRoutineWithIoManager(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID  Context) {
    DbgPrint("Example_SampleCompletionRoutineWithIoManager\n");

    return STATUS_CONTINUE_COMPLETION ;
}
