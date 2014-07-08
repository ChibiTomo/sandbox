#ifndef __FILTER_H__
#define __FILTER_H__

typedef unsigned int UINT;
typedef char * PCHAR;

/* #define __USE_DIRECT__ */
#define __USE_BUFFERED__

NTSTATUS STDCALL ExampleFilter_Create(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL ExampleFilter_Close(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL ExampleFilter_IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL ExampleFilter_Write(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL ExampleFilter_Read(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL ExampleFilter_UnSupportedFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL ExampleFilter_IoControlInternal(PDEVICE_OBJECT DeviceObject, PIRP Irp);

typedef struct _EXAMPLE_FILTER_EXTENSION
{
    PDEVICE_OBJECT pNextDeviceInChain;

} EXAMPLE_FILTER_EXTENSION, *PEXAMPLE_FILTER_EXTENSION;


#define EXAMPLE_FILTER_POOL_TAG ((ULONG) 'pxEF')


#endif // __FILTER_H__






