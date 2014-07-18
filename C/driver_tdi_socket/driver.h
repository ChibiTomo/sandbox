#ifndef DRIVER_H
#define DRIVER_H

#include <ntddk.h>
#include <tdikrnl.h>
#include "public.h"
#include "tdi.h"

// Missing definitions
#define OBJ_KERNEL_HANDLE 0x00000200L

#define MY_POOL_TAG ' gaT'

typedef struct {
	tdi_handle_t handle;
} file_extension_t;


VOID STDCALL my_unload(PDRIVER_OBJECT driverObject);
NTSTATUS STDCALL my_unsupported_function(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_create(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_close(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_read(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_write(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp);

NTSTATUS my_ioctl_tdi_connect(file_extension_t* context, PIRP Irp);

#endif // DRIVER_H