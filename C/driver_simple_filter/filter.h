#ifndef FILTER_H
#define FILTER_H

#define DbgPrint(format, ...) DbgPrint("[Filter] "format, ##__VA_ARGS__)

typedef struct {
	PDEVICE_OBJECT next;
} device_extension_t;

VOID STDCALL my_unload(PDRIVER_OBJECT driverObject);
NTSTATUS STDCALL my_ignored_function(PDEVICE_OBJECT deviceObject, PIRP Irp);

#endif // FILTER_H