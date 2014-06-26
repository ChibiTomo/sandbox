NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath);
VOID STDCALL my_unload(PDRIVER_OBJECT DriverObject);
NTSTATUS STDCALL unsuported_function(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL my_create(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL my_close(PDEVICE_OBJECT DeviceObject, PIRP Irp);

#define POOL_TAG ' pxE'
#define CIRCULAR_BUFFER_SIZE 2048

typedef struct _file_info_t{
	struct _file_info_t* next;
	KMUTEX mutex;
	UNICODE_STRING fname;
	WCHAR unicode_fname[256];
	int ref_count;

	int start_index;
	int end_index;
	char* circular_buffer[CIRCULAR_BUFFER_SIZE];
} file_info_t;

typedef struct {
	KMUTEX mutex;
	file_info_t* info_list;
} device_context_t;