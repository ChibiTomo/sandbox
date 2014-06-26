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

	switch (pIoStackIrp->Parameters.DeviceIoControl.IoControlCode) {
		case MY_IOCTL_DIRECT_IN_IO:
			NtStatus = my_ioctl_direct_in(Irp, pIoStackIrp, &dwDataWritten);
			break;

		case MY_IOCTL_DIRECT_OUT_IO:
			NtStatus = my_ioctl_direct_out(Irp, pIoStackIrp, &dwDataWritten);
			break;

		case MY_IOCTL_BUFFERED_IO:
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

NTSTATUS my_ioctl_direct_out(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("my_ioctl_direct_out Called \n");

	PCHAR pInputBuffer = Irp->AssociatedIrp.SystemBuffer;
	if (!pInputBuffer) {
		DbgPrint("No input buffer.\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PCHAR pOutputBuffer = NULL;
	if (Irp->MdlAddress) {
		pOutputBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	}

	if (!pOutputBuffer) {
		DbgPrint("No output buffer.\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	DbgPrint("pOutputBuffer: %s\n", pOutputBuffer);

	if (!isStrNullTerminated(pInputBuffer, pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength)) {
		DbgPrint("Not null terminated string.\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	DbgPrint("UserModeMessage: %s\n", pInputBuffer);

	PCHAR pReturnData = "IOCTL - Direct Out I/O From Kernel!";
	UINT dwDataSize = strlen(pReturnData) + 1;
	UINT returnDataSize = dwDataSize + strlen(pOutputBuffer);

	DbgPrint("%i >= %i?\n", pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength, returnDataSize);
	if(pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength >= returnDataSize) {
		RtlCopyMemory(pOutputBuffer + strlen(pOutputBuffer), pReturnData, dwDataSize);

		*pdwDataWritten = returnDataSize;
	} else {
		*pdwDataWritten = 0;
		NtStatus = STATUS_BUFFER_TOO_SMALL;
	}

cleanup:
	return NtStatus;
}

NTSTATUS my_ioctl_direct_in(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("my_ioctl_direct_out Called \n");

	PCHAR pInputBuffer = Irp->AssociatedIrp.SystemBuffer;
	if (!pInputBuffer) {
		DbgPrint("No input buffer.\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	PCHAR pOutputBuffer = NULL;
	if (Irp->MdlAddress) {
		pOutputBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	}

	if (!pOutputBuffer) {
		DbgPrint("No output buffer.\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	DbgPrint("pOutputBuffer: %s\n", pOutputBuffer);

	if (!isStrNullTerminated(pInputBuffer, pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength)) {
		DbgPrint("Not null terminated string.\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	DbgPrint("UserModeMessage: %s\n", pInputBuffer);

	PCHAR pReturnData = "IOCTL - Direct In I/O From Kernel!";
	UINT dwDataSize = strlen(pReturnData) + 1;
	UINT returnDataSize = dwDataSize + strlen(pOutputBuffer);

	DbgPrint("%i >= %i?\n", pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength, returnDataSize);
	if(pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength >= returnDataSize) {
		RtlCopyMemory(pOutputBuffer + strlen(pOutputBuffer), pReturnData, dwDataSize);

		*pdwDataWritten = returnDataSize;
	} else {
		*pdwDataWritten = 0;
		NtStatus = STATUS_BUFFER_TOO_SMALL;
	}

cleanup:
	return NtStatus;
}

NTSTATUS my_ioctl_buffered(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten) {
	NTSTATUS NtStatus = STATUS_SUCCESS;

	DbgPrint("my_ioctl_buffered called \r\n");

	PCHAR buffer = Irp->AssociatedIrp.SystemBuffer;

	if(!buffer) {
		DbgPrint("No buffer.\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	if(!isStrNullTerminated(buffer, pIoStackIrp->Parameters.DeviceIoControl.InputBufferLength)) {
		DbgPrint("Not null terminated string.\n");
		NtStatus = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	DbgPrint("UserModeMessage = %s\n", buffer);

	PCHAR pReturnData = "IOCTL - Buffered I/O From Kernel!";
	UINT dwDataSize = strlen(pReturnData) + 1;

	DbgPrint("%i >= %i\n", pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength, dwDataSize);
	if(pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength >= dwDataSize) {
		RtlCopyMemory(buffer, pReturnData, dwDataSize);
		*pdwDataWritten = dwDataSize;
	} else {
		*pdwDataWritten = 0;
		NtStatus = STATUS_BUFFER_TOO_SMALL;
	}

cleanup:
	return NtStatus;
}