#ifndef FILTER_H
#define FILTER_H

#define DbgPrint(format, ...) DbgPrint("[Filter] "format, ##__VA_ARGS__)

typedef struct {
	PDEVICE_OBJECT next;
} device_extension_t;

VOID STDCALL my_unload(PDRIVER_OBJECT driverObject);
NTSTATUS STDCALL my_ignored_function(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS my_completion_routine(PDEVICE_OBJECT deviceObject, PIRP Irp, PVOID extension);

NTSTATUS my_ioctl_push(PIRP Irp);
NTSTATUS my_ioctl_pop(PDEVICE_OBJECT deviceObject, PIRP Irp);

#endif // FILTER_H