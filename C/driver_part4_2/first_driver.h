#ifndef DRIVER_H
#define DRIVER_H

#define DbgPrint(format, ...) DbgPrint("[First_Driver] "format, ##__VA_ARGS__)

VOID STDCALL my_unload(PDRIVER_OBJECT driverObject);
NTSTATUS STDCALL my_unsuported_function(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_create(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_close(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_internal_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp);

NTSTATUS my_internal_ioctl_say_hello(PIRP Irp);

#endif // DRIVER_H