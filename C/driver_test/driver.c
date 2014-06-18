#include <ntddk.h>

VOID STDCALL my_unload(PDRIVER_OBJECT DriverObject) {
	DbgPrint("GoodBye!!\n");

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

NTSTATUS STDCALL my_io_control(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("my_io_control called \n");
	return status;
}

NTSTATUS STDCALL my_read(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint("my_read called \n");
	return status;
}

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
	driverObject->MajorFunction[IRP_MJ_READ] = my_read;
	//driverObject->MajorFunction[IRP_MJ_WRITE] = USE_WRITE_FUNCTION;

	// Setting my_unload as unload function
	driverObject->DriverUnload = my_unload;

cleanup:
	return status;
}