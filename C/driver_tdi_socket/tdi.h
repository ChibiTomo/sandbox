#ifndef TDI_H
#define TDI_H

#include <ntddk.h>
#include <tdikrnl.h>
#include "public.h"

// Missing definitions
#define OBJ_KERNEL_HANDLE 0x00000200L

#define MY_POOL_TAG ' gaT'

typedef struct _TDI_HANDLE {
	HANDLE transport_handle;
	HANDLE connection_handle;
	PFILE_OBJECT transport;
	PFILE_OBJECT connection;
} tdi_handle_t;

NTSTATUS tdi_initialize_handle(tdi_handle_t* file_context, PIO_STACK_LOCATION pIoStackIrp);
NTSTATUS tdi_open_transport_address(PHANDLE transport_handle, PFILE_OBJECT* transport);
NTSTATUS tdi_open_connection(PHANDLE connection_handle, PFILE_OBJECT* connection);
NTSTATUS tdi_create_handle_file_object(	PHANDLE phandle,
									PFILE_OBJECT* ppfile_object,
									PFILE_FULL_EA_INFORMATION exAttrInfo,
									int dataSize);
NTSTATUS tdi_associate_transport_connection(HANDLE transport_handle, PFILE_OBJECT connection);
NTSTATUS tdi_disassociate_transport_connection(PFILE_OBJECT connection);
void tdi_free_handle(tdi_handle_t* tdi_handle);
NTSTATUS tdi_close_handle(HANDLE handle, PFILE_OBJECT file);

#endif // TDI_H