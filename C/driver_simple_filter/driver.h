#ifndef DRIVER_H
#define DRIVER_H

#define DbgPrint(format, ...) DbgPrint("[Driver] "format, ##__VA_ARGS__)

VOID STDCALL my_unload(PDRIVER_OBJECT driverObject);
NTSTATUS STDCALL my_unsuported_function(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_create(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp);

NTSTATUS my_ioctl_push(PIRP Irp, UINT* dataSize);
NTSTATUS my_ioctl_pop(PIRP Irp, UINT* dataSize);

#endif // DRIVER_H