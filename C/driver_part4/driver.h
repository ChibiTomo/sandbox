#ifndef __DRIVER_H__
#define __DRIVER_H__

typedef unsigned int UINT;
typedef char * PCHAR;

/* #define __USE_DIRECT__  */
#define __USE_BUFFERED__

NTSTATUS STDCALL Example_Create(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL Example_Close(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL Example_IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL Example_WriteBufferedIO(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL Example_WriteDirectIO(PDEVICE_OBJECT DeviceObject, PIRP Irp);
//NTSTATUS STDCALL Example_WriteNeither(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL Example_ReadBufferedIO(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL Example_ReadDirectIO(PDEVICE_OBJECT DeviceObject, PIRP Irp);
//NTSTATUS STDCALL Example_ReadNeither(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL Example_UnSupportedFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS STDCALL Example_IoControlInternal(PDEVICE_OBJECT DeviceObject, PIRP Irp);


#ifdef __USE_DIRECT__
#define IO_TYPE DO_DIRECT_IO
#define USE_WRITE_FUNCTION  Example_WriteDirectIO
#define USE_READ_FUNCTION   Example_ReadDirectIO
#endif

#ifdef __USE_BUFFERED__
#define IO_TYPE DO_BUFFERED_IO
#define USE_WRITE_FUNCTION  Example_WriteBufferedIO
#define USE_READ_FUNCTION   Example_ReadBufferedIO
#endif

#ifndef IO_TYPE
#define IO_TYPE 0
#define USE_WRITE_FUNCTION  Example_WriteNeither
#define USE_READ_FUNCTION   Example_ReadNeither
#endif


#define EXAMPLE_POOL_TAG ((ULONG)' pxE')
/*
 * This is a list of connected handles
 */
typedef struct _EXAMPLE_LIST
{
    struct _EXAMPLE_LIST *pNext;
    UNICODE_STRING usPipeName;
    WCHAR szwUnicodeString[256];
    UINT uiRefCount;
    UINT uiStartIndex;
    UINT uiStopIndex;
    KMUTEX kInstanceBufferMutex;
    char pCircularBuffer[20];

} EXAMPLE_LIST, *PEXAMPLE_LIST;


/*
 * Context that is user-defined, the developer
 * can create any type of device structure that they
 * need.  This structure can be uniquely defined for each created
 * device object.
 *
 */

typedef struct _EXAMPLE_DEVICE_CONTEXT
{
    PEXAMPLE_LIST pExampleList;
    KMUTEX kListMutex;

} EXAMPLE_DEVICE_CONTEXT, *PEXAMPLE_DEVICE_CONTEXT;

BOOLEAN STDCALL Example_WriteData(PEXAMPLE_LIST pExampleList, PCHAR pData, UINT uiLength, UINT *pdwStringLength);
BOOLEAN STDCALL Example_ReadData(PEXAMPLE_LIST pExampleList, PCHAR pData, UINT uiLength, UINT *pdwStringLength);
BOOLEAN STDCALL Example_IsStringTerminated(PCHAR pString, UINT uiLength, UINT *pdwStringLength);
NTSTATUS STDCALL Example_HandleSampleIoctl_DirectInIo(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten);
NTSTATUS STDCALL Example_HandleSampleIoctl_DirectOutIo(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten);
NTSTATUS STDCALL Example_HandleSampleIoctl_BufferedIo(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten);
//NTSTATUS STDCALL Example_HandleSampleIoctl_NeitherIo(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten);
NTSTATUS STDCALL Example_CreatePipeContext(PEXAMPLE_DEVICE_CONTEXT pExampleDeviceContext, PFILE_OBJECT pFileObject);
NTSTATUS STDCALL Example_ReleasePipeContext(PEXAMPLE_DEVICE_CONTEXT pExampleDeviceContext, PFILE_OBJECT pFileObject);
NTSTATUS STDCALL Example_CreateNewResource(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten);
NTSTATUS STDCALL Example_DestroyResource(PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten);
NTSTATUS STDCALL Example_SampleCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID  Context);
NTSTATUS STDCALL Example_SampleCompletionRoutineWithIoManager(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID  Context);

#endif //__DRIVER_H__






