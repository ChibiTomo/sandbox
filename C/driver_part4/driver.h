#define POOL_TAG ' pxE'
#define CIRCULAR_BUFFER_SIZE 2048

#define MY_IOCTL_INTERNAL_CREATE_EXTENSION \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

#define MY_IOCTL_INTERNAL_CLOSE_EXTENSION \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)


typedef struct _file_extension_t{
	struct _file_extension_t* next;
	KMUTEX mutex;
	UNICODE_STRING fname;
	WCHAR unicode_fname[256];
	int ref_count;

	int start_index;
	int end_index;
	char* circular_buffer[CIRCULAR_BUFFER_SIZE];
} file_extension_t;

typedef struct {
	KMUTEX mutex;
	file_extension_t* extensions_list;
} device_extension_t;

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath);
VOID STDCALL my_unload(PDRIVER_OBJECT DriverObject);
NTSTATUS STDCALL unsuported_function(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL my_create(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL my_close(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL my_internal_ioctl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL my_read(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL my_write(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS create_pipe_extension(device_extension_t* deviceContext, PFILE_OBJECT pFileObject);
NTSTATUS release_pipe_extension(device_extension_t* deviceContext, PFILE_OBJECT pFileObject);

NTSTATUS create_extension(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten);
NTSTATUS close_extension(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten);

NTSTATUS my_completion_routine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);
