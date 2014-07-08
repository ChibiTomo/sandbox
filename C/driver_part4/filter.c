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
//    NTSTATUS NtStatus = STATUS_BUFFER_TOO_SMALL;
//    PEXAMPLE_FILTER_EXTENSION pExampleFilterDeviceContext = (PEXAMPLE_FILTER_EXTENSION)DeviceObject->DeviceExtension;
//    PIO_STACK_LOCATION pIoStackIrp = NULL;
//    PCHAR pReadDataBuffer;
//
//    DbgPrint("ExampleFilter_Read Called \r\n");
//
//    /*
//     * We want to process this IRP.  Our filter driver's functionality is to remove all NULL's from the
//     * returned strings and add one only to the end of the string returned.  This ensures that if we
//     * return strings without NULL's that they are given NULL's and if we provide an array of NULL terminated
//     * strings that they are all displayed.  This is essentially the functionality we will add in this
//     * simple example.
//     *
//     * When we want to process an IRP after we pass it down we need to do two things the first is we need to
//     * call IoCopyCurrentIrpStackLocationToNext() and set a completetion routine.
//     */
//    pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
//
//    IoCopyCurrentIrpStackLocationToNext(Irp);
//    IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) ExampleFilter_CompletionRoutine, NULL, TRUE, TRUE, TRUE);
//
//   /*
//    * IoCallDriver() simply calls the appropriate entry point in the driver object associated
//    * with the device object.  This is how drivers are basically "chained" together, they must know
//    * that there are lower driver so they can perform the appropriate action and send down the IRP.
//    *
//    * They do not have to send the IRP down they could simply process it completely themselves if they wish.
//    */
//
//    NtStatus = IoCallDriver(pExampleFilterDeviceContext->pNextDeviceInChain, Irp);
//
//    /*
//     * Please note that our implementation here is a simple one.  We do not take into account PENDING IRP's or
//     * anything complicated.  We assume that once we get to this locaiton the IRP has already been completed and
//     * our completetion routine was called or it wasn't completed and we are still able to complete it here.
//     *
//     * Our completetion routine makes sure that the IRP is still valid here.
//     *
//     */
//
//    if(NT_SUCCESS(NtStatus))
//    {
//        /*
//         * Data was read?
//         */
//        if(Irp->IoStatus.Information)
//        {
//            /*
//             * Our filter device is dependent upon the compliation settings of how we compiled example.sys
//             * That means we need to dynamically figure out if we're using Direct, Buffered or Neither.
//             */
//            if(DeviceObject->Flags & DO_BUFFERED_IO)
//            {
//
//                    DbgPrint("ExampleFilter_Read - Use Buffered I/O \r\n");
//                    /*
//                     * Implementation for Buffered I/O
//                     */
//
//                    pReadDataBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;
//
//                    if(pReadDataBuffer && pIoStackIrp->Parameters.Read.Length > 0)
//                    {
//                        ExampleFilter_FixNullString(pReadDataBuffer, (UINT)Irp->IoStatus.Information);
//                    }
//            }
//            else
//            {
//                if(DeviceObject->Flags & DO_DIRECT_IO)
//                {
//                    DbgPrint("ExampleFilter_Read - Use Direct I/O \r\n");
//                    /*
//                     * Implementation for Direct I/O
//                     */
//                    if(pIoStackIrp && Irp->MdlAddress)
//                    {
//                        pReadDataBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
//
//                        if(pReadDataBuffer && pIoStackIrp->Parameters.Read.Length)
//                        {
//                            ExampleFilter_FixNullString(pReadDataBuffer, (UINT)Irp->IoStatus.Information);
//                        }
//                    }
//                }
//                else
//                {
//
//                    DbgPrint("ExampleFilter_Read - Use Neither I/O \r\n");
//
//                    /*
//                     * Implementation for Neither I/O
//                     */
//                    __try {
//
//                            if(pIoStackIrp->Parameters.Read.Length > 0 && Irp->UserBuffer)
//                            {
//
//                                ProbeForWrite(Irp->UserBuffer, pIoStackIrp->Parameters.Read.Length, TYPE_ALIGNMENT(char));
//                                pReadDataBuffer = Irp->UserBuffer;
//
//                                ExampleFilter_FixNullString(pReadDataBuffer, (UINT)Irp->IoStatus.Information);
//                            }
//
//                        } __except( EXCEPTION_EXECUTE_HANDLER ) {
//
//                              NtStatus = GetExceptionCode();
//                        }
//                }
//            }
//
//        }
//    }
//
//    /*
//     * Complete the IRP
//     *
//     */

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

