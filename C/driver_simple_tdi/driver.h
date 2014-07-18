#ifndef DRIVER_H
#define DRIVER_H

#include <ntddk.h>
#include <tdikrnl.h>
#include "public.h"

typedef struct {
	PDEVICE_OBJECT topOfDeviceStack ;
} extension_t;

typedef struct {
	unsigned char address[4];
	unsigned short port;
} network_address_t;

VOID STDCALL my_unload(PDRIVER_OBJECT driverObject);
NTSTATUS STDCALL my_ignored_function(PDEVICE_OBJECT deviceObject, PIRP Irp);
NTSTATUS STDCALL my_internal_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp);

NTSTATUS process_tdi_connect(PDEVICE_OBJECT deviceObject, PIRP Irp);
void convert_to_network_address(network_address_t* network_address, PTDI_ADDRESS_IP address_ip);
void set_network_address(PTDI_ADDRESS_IP address_ip, network_address_t* address);

#endif // DRIVER_H