#ifndef DRIVER_H
#define DRIVER_H

#include <ntddk.h>
#include <tdikrnl.h>
#include "public.h"

typedef struct {
	PDEVICE_OBJECT topOfDeviceStack ;
} extension_t;

VOID STDCALL my_unload(PDRIVER_OBJECT driverObject);
NTSTATUS STDCALL my_ignored_function(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_internal_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp);

#endif // DRIVER_H