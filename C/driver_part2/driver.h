#ifndef DRIVER_H
#define DRIVER_H

#include <ntddk.h>

VOID STDCALL my_unload(PDRIVER_OBJECT DriverObject);
NTSTATUS STDCALL unsuported_function(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL my_close(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL my_create(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL my_io_control(PDEVICE_OBJECT DeviceObject, PIRP Irp);


NTSTATUS STDCALL my_ioctl_direct_out(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten);
NTSTATUS STDCALL my_ioctl_direct_in(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten);
NTSTATUS STDCALL my_ioctl_buffered(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten);

BOOLEAN isStrNullTerminated(PCHAR pString, UINT uiLength);

#ifdef __USE_DIRECT__
#define IO_TYPE DO_DIRECT_IO
#else
#define IO_TYPE DO_BUFFERED_IO
#endif // __USE_DIRECT__

#endif // DRIVER_H