#include <ntddk.h>

VOID STDCALL my_unload(PDRIVER_OBJECT DriverObject) {
	DbgPrint("GoodBye!!\n");

	PDEVICE_OBJECT device = DriverObject->DeviceObject;
	if (device != NULL) {
		IoDeleteDevice(device);
	}
}

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS; // default status

	// Debug print
	DbgPrint("Hello!!\n");
	DbgPrint("RegistryPath=%wZ\n", registryPath);

	// Setting my_unload as unload function
	driverObject->DriverUnload = my_unload;

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

cleanup:
	return status;
}