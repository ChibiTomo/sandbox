#ifndef PUBLIC_H
#define PUBLIC_H

#define DEBUG(format, ...) DEBUG_FUNC("[" __FILE__ ":%d] " format, __LINE__, ##__VA_ARGS__)

#ifndef __USE_NTOSKRNL__ // Not inside the driver (ie: in client app)
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

#define DEBUG_FUNC printf

#else // Inside Driver

#define DEBUG_FUNC DbgPrint

#endif // __USE_NTOSKRNL__

#define NICKNAME_SIZE 126

#define DEVICE_NAME "YTDevice"
#define DEVICE_PATH L"\\Device\\" DEVICE_NAME
#define DOSDEVICE_PATH L"\\DosDevices\\" DEVICE_NAME
#define FILE_PATH "\\\\.\\" DEVICE_NAME

#define MY_IOCTL_TDI_CONNECT \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

typedef struct {
	unsigned long address;
	unsigned short port;
} network_address_t;

#endif // PUBLIC_H