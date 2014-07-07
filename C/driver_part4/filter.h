typedef struct {
	PDEVICE_OBJECT nextDeviceInChain;
} filter_device_extension_t;

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath);
VOID STDCALL my_unload(PDRIVER_OBJECT DriverObject);
NTSTATUS STDCALL unsuported_function(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL ignored_function(PDEVICE_OBJECT DeviceObject, PIRP Irp);