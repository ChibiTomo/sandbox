#ifndef DRIVER_H
#define DRIVER_H

#define DbgPrint(format, ...) DbgPrint("[Driver] "format, ##__VA_ARGS__)

#define MY_POOL_TAG ' gaT'

typedef struct _my_stack_t {
	char c;
	struct _my_stack_t* next;
} my_stack_t;

typedef struct {
	KMUTEX mutex;
	my_stack_t* stack;
} device_context_t;

VOID STDCALL my_unload(PDRIVER_OBJECT driverObject);
NTSTATUS STDCALL my_unsuported_function(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_create(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_close(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp);

NTSTATUS my_ioctl_push(device_context_t* context, PIRP Irp);
NTSTATUS my_ioctl_pop(device_context_t* context, PIRP Irp);

#endif // DRIVER_H