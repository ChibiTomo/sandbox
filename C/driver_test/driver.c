#include <ntddk.h>

#define TYPE_ALIGNMENT(type) offsetof(struct {char x; type t;}, t)

#include "driver.h"
#include "public.h"

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS; // default status

	// Debug print
	DbgPrint("Hello!!\n");
	DbgPrint("RegistryPath=%wZ\n", registryPath);

	// Creation of the associated device
	PDEVICE_OBJECT deviceObject = NULL; // represents a logical, virtual, or physical device for which a driver handles I/O requests.
	UNICODE_STRING deviceName;
	UNICODE_STRING dosDeviceName;// unicode string struct:	USHORT Length, current length of the string
								//							USHORT MaximumLength, max length of the string
								//							PWSTR  Buffer, the string /!\not even NULL terminated!

	RtlInitUnicodeString(&deviceName, L"\\Device\\Example"); // Initialize a unicode string
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\Example");

	status = IoCreateDevice(driverObject, 			// driver object
							0, 						// length of device extention (extra data to pass)
							&deviceName, 			// device name
							FILE_DEVICE_UNKNOWN, 	// unknow because it's not a specific device
							FILE_DEVICE_SECURE_OPEN,// flags:	FILE_DEVICE_SECURE_OPEN,
													//			FILE_FLOPPY_DISKETTE,
													//			FILE_READ_ONLY_DEVICE,
													//			FILE_REMOVABLE_MEDIA,
													//			FILE_WRITE_ONCE_MEDIA
							FALSE, 					// is an exclusive device?
							&deviceObject);			// out

	if (status != STATUS_SUCCESS) {
		IoDeleteDevice(deviceObject);
		goto cleanup;
	}

	for (UINT i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		driverObject->MajorFunction[i] = unsuported_function;
	}
	driverObject->MajorFunction[IRP_MJ_CLOSE] = my_close;
	driverObject->MajorFunction[IRP_MJ_CREATE] = my_create;
	driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = my_io_control;
	driverObject->MajorFunction[IRP_MJ_READ] = USE_READ_FUNCTION;
	driverObject->MajorFunction[IRP_MJ_WRITE] = USE_WRITE_FUNCTION;

	// Setting my_unload as unload function
	driverObject->DriverUnload = my_unload;

	deviceObject->Flags |= IO_TYPE;
	deviceObject->Flags &= (~DO_DEVICE_INITIALIZING);	// DO_DEVICE_INITIALIZING: tell to not send I/O request to
														// the device. It is MANDATORY to clear it to use the device.
														// (except in DriverEntry because it is automatic)

	IoCreateSymbolicLink(&dosDeviceName, &deviceName);

cleanup:
	return status;
}

VOID STDCALL my_unload(PDRIVER_OBJECT DriverObject) {
	DbgPrint("GoodBye!!\n");

	// Remove SymLinks
	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, L"\\DosDevices\\Example");
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

NTSTATUS STDCALL my_close(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("my_close called \n");
	return status;
}

NTSTATUS STDCALL my_create(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("my_create called \n");
	return status;
}

NTSTATUS STDCALL my_write_direct(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("my_write_direct called \n");

	PCHAR pWriteDataBuffer;

	/*
	* Each time the IRP is passed down
	* the driver stack a new stack location is added
	* specifying certain parameters for the IRP to the driver.
	*/
	PIO_STACK_LOCATION pIoStackIrp = NULL;
	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);

	if (!pIoStackIrp) {
		goto cleanup;
	}

// SPECIALIZED PART
	pWriteDataBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
// SPECIALIZED PART END

	if (!pWriteDataBuffer) {
		goto cleanup;
	}

	/*
	* We need to verify that the string
	* is NULL terminated. Bad things can happen
	* if we access memory not valid while in the Kernel.
	*/
	if(isStrNullTerminated(pWriteDataBuffer, pIoStackIrp->Parameters.Write.Length)) {
		DbgPrint(pWriteDataBuffer);
	}

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = strlen(pWriteDataBuffer);
	/*
	 * /!\MANDATORY after MmGetSystemAddressForMdlSafe/!\
	 * Tell to the I/O Manager that the driver has finish to work with the IRP
	 * Can lead to BSOD if not called
	 *
	 * http://msdn.microsoft.com/en-us/library/windows/hardware/ff565381(v=vs.85).aspx
	**/
	IoCompleteRequest(	Irp, 				// pointer to the IRP
						IO_NO_INCREMENT);	// priority, system-defined constant. Check ntddk.h

	return status;
}

NTSTATUS STDCALL my_write_buffered(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("my_write_buffered called\n");

	PIO_STACK_LOCATION pIoStackIrp = NULL;
	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	PCHAR pWriteDataBuffer;

	if(!pIoStackIrp) {
		goto cleanup;
	}

// SPECIALIZED PART
	pWriteDataBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;
// SPECIALIZED PART END

	if(!pWriteDataBuffer) {
		goto cleanup;
	}
	/*
	* We need to verify that the string
	* is NULL terminated. Bad things can happen
	* if we access memory not valid while in the Kernel.
	*/
	if(isStrNullTerminated(pWriteDataBuffer, pIoStackIrp->Parameters.Write.Length)) {
		DbgPrint(pWriteDataBuffer);
	}

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = strlen(pWriteDataBuffer);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS STDCALL my_read_direct(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_BUFFER_TOO_SMALL;
	DbgPrint("my_read_direct called \n");

	PIO_STACK_LOCATION pIoStackIrp = NULL;
	PCHAR pReturnData = "Hello from the Kernel!";
	UINT dwDataSize = strlen(pReturnData);
	UINT dwDataRead = 0;
	PCHAR pReadDataBuffer;
	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);

	if(!pIoStackIrp || !Irp->MdlAddress) {
		goto cleanup;
	}
// SPECIALIZED PART
	pReadDataBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
// SPECIALIZED PART END

	if(!pReadDataBuffer || pIoStackIrp->Parameters.Read.Length < dwDataSize) {
		goto cleanup;
	}
	/*
	* We use "RtlCopyMemory" in the kernel instead
	* of memcpy.
	* RtlCopyMemory *IS* memcpy, however it's best
	* to use the
	* wrapper in case this changes in the future.
	*/
	RtlCopyMemory(pReadDataBuffer, pReturnData, dwDataSize);
	dwDataRead = dwDataSize;
	status = STATUS_SUCCESS;

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = dwDataRead;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS STDCALL my_read_buffered(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_BUFFER_TOO_SMALL;
	DbgPrint("my_read_buffered called \n");

	PIO_STACK_LOCATION pIoStackIrp = NULL;
	PCHAR pReturnData = "Hello from the Kernel!";
	UINT dwDataSize = strlen(pReturnData);
	UINT dwDataRead = 0;
	PCHAR pReadDataBuffer;

	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);

	if(!pIoStackIrp) {
		DbgPrint("No Irp pointer\n");
		goto cleanup;
	}
// SPECIALIZED PART
	pReadDataBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;
// SPECIALIZED PART END

	if(!pReadDataBuffer || pIoStackIrp->Parameters.Read.Length < dwDataSize) {
		DbgPrint("No data buffer || buffer size too small\n");
		goto cleanup;
	}
	/*
	* We use "RtlCopyMemory" in the kernel instead
	* of memcpy.
	* RtlCopyMemory *IS* memcpy, however it's best
	* to use the
	* wrapper in case this changes in the future.
	*/
	RtlCopyMemory(pReadDataBuffer, pReturnData, dwDataSize);
	dwDataRead = dwDataSize;
	status = STATUS_SUCCESS;

cleanup:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = dwDataRead;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}

BOOLEAN isStrNullTerminated(PCHAR str, UINT length) {
	BOOLEAN result = FALSE;

	UINT i = 0;
	while (i < length && result == FALSE) {
		if(str[i] == '\0') {
			result = TRUE;
		} else {
			i++;
		}
	}

	DbgPrint("result=%d\n", result);
	return result;
}

NTSTATUS STDCALL my_io_control(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS NtStatus = STATUS_NOT_SUPPORTED;
	PIO_STACK_LOCATION pIoStackIrp = NULL;
	UINT dwDataWritten = 0;

	DbgPrint("my_ioControl called \r\n");

	pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);

	if(!pIoStackIrp) {
		goto cleanup;
	}

	switch(pIoStackIrp->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_DIRECT_IN_IO:
			NtStatus = my_ioctl_direct_in(Irp, pIoStackIrp, &dwDataWritten);
			break;

		case IOCTL_DIRECT_OUT_IO:
			NtStatus = my_ioctl_direct_out(Irp, pIoStackIrp, &dwDataWritten);
			break;

		case IOCTL_BUFFERED_IO:
			NtStatus = my_ioctl_buffered(Irp, pIoStackIrp, &dwDataWritten);
			break;

//		case IOCTL_EXAMPLE_SAMPLE_NEITHER_IO:
//			NtStatus = Example_HandleSampleIoctl_NeitherIo(Irp,
//			pIoStackIrp, &dwDataWritten);
//			break;
	}

cleanup:
	Irp->IoStatus.Status = NtStatus;
	Irp->IoStatus.Information = dwDataWritten;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return NtStatus;
}

NTSTATUS STDCALL my_ioctl_direct_out(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten) {
	NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
	PCHAR pInputBuffer;
	PCHAR pOutputBuffer;
	UINT dwDataRead = 0;
	UINT dwDataWritten = 0;
	PCHAR pReturnData = "IOCTL - Direct Out I/O From Kernel!";
	UINT dwDataSize = strlen(pReturnData);
	DbgPrint("my_ioctl_direct_out Called \n");

	pInputBuffer = Irp->AssociatedIrp.SystemBuffer;
	pOutputBuffer = NULL;

	if(Irp->MdlAddress) {
		pOutputBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	}

	if(!pInputBuffer || !pOutputBuffer) {
		goto cleanup;
	}

	/*
	* We need to verify that the string
	* is NULL terminated. Bad things can happen
	* if we access memory not valid while in the Kernel.
	*/
	if(!isStrNullTerminated(pInputBuffer, pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength)) {
		goto cleanup;
	}
	DbgPrint("UserModeMessage: %s\n", pInputBuffer);
	DbgPrint("%i >= %i\n", pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength, dwDataSize);
	if(pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength >= dwDataSize) {
		/*
		* We use "RtlCopyMemory" in the kernel instead of memcpy.
		* RtlCopyMemory *IS* memcpy, however it's best to use the
		* wrapper in case this changes in the future.
		*/
		RtlCopyMemory(pOutputBuffer, pReturnData, dwDataSize);
		*pdwDataWritten = dwDataSize;
		NtStatus = STATUS_SUCCESS;
	} else {
		*pdwDataWritten = dwDataSize;
		NtStatus = STATUS_BUFFER_TOO_SMALL;
	}

cleanup:
	return NtStatus;
}

NTSTATUS STDCALL my_ioctl_direct_in(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten) {
	NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
	PCHAR pInputBuffer;
	PCHAR pOutputBuffer;
	UINT dwDataRead = 0;
	UINT dwDataWritten = 0;
	PCHAR pReturnData = "IOCTL - Direct In I/O From Kernel!";
	UINT dwDataSize = strlen(pReturnData);
	DbgPrint("my_ioctl_direct_in Called \n");

	pInputBuffer = Irp->AssociatedIrp.SystemBuffer;
	pOutputBuffer = NULL;

	if(Irp->MdlAddress) {
		pOutputBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	}

	if(!pInputBuffer || !pOutputBuffer) {
		goto cleanup;
	}

	/*
	* We need to verify that the string
	* is NULL terminated. Bad things can happen
	* if we access memory not valid while in the Kernel.
	*/
	if(!isStrNullTerminated(pInputBuffer, pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength)) {
		goto cleanup;
	}
	DbgPrint("UserModeMessage: %s\n", pInputBuffer);
	DbgPrint("%i >= %i\n", pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength, dwDataSize);
	if(pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength >= dwDataSize) {
		/*
		* We use "RtlCopyMemory" in the kernel instead of memcpy.
		* RtlCopyMemory *IS* memcpy, however it's best to use the
		* wrapper in case this changes in the future.
		*/
		RtlCopyMemory(pOutputBuffer, pReturnData, dwDataSize);
		*pdwDataWritten = dwDataSize;
		NtStatus = STATUS_SUCCESS;
	} else {
		*pdwDataWritten = dwDataSize;
		NtStatus = STATUS_BUFFER_TOO_SMALL;
	}

cleanup:
	return NtStatus;
}

NTSTATUS STDCALL my_ioctl_buffered(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten) {
	NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
	PCHAR pInputBuffer;
	PCHAR pOutputBuffer;
	UINT dwDataRead = 0;
	UINT dwDataWritten = 0;
	PCHAR pReturnData = "IOCTL - Buffered I/O From Kernel!";
	UINT dwDataSize = strlen(pReturnData);
	DbgPrint("my_ioctl_buffered called \r\n");

	/*
	* METHOD_BUFFERED
	*
	*    Input Buffer = Irp->AssociatedIrp.SystemBuffer
	*    Ouput Buffer = Irp->AssociatedIrp.SystemBuffer
	*
	*    Input Size   =  Parameters.DeviceIoControl.InputBufferLength
	*    Output Size  =  Parameters.DeviceIoControl.OutputBufferLength
	*
	*    Since they both use the same location
	*    so the "buffer" allocated by the I/O
	*    manager is the size of the larger value (Output vs. Input)
	*/


	pInputBuffer = Irp->AssociatedIrp.SystemBuffer;
	pOutputBuffer = Irp->AssociatedIrp.SystemBuffer;

	if(!pInputBuffer || !pOutputBuffer) {
		goto cleanup;
	}
	/*
	* We need to verify that the string
	* is NULL terminated. Bad things can happen
	* if we access memory not valid while in the Kernel.
	*/
	if(!isStrNullTerminated(pInputBuffer, pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength)) {
		goto cleanup;
	}
	DbgPrint("UserModeMessage = %s\n", pInputBuffer);
	DbgPrint("%i >= %i\n", pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength, dwDataSize);
	if(pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength >= dwDataSize) {
		RtlCopyMemory(pOutputBuffer, pReturnData, dwDataSize);
		*pdwDataWritten = dwDataSize;
		NtStatus = STATUS_SUCCESS;
	} else {
		*pdwDataWritten = dwDataSize;
		NtStatus = STATUS_BUFFER_TOO_SMALL;
	}

cleanup:
	return NtStatus;
}