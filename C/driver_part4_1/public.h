#ifndef PUBLIC_H
#define PUBLIC_H

#ifndef __USE_NTOSKRNL__ // Not inside the driver (ie: in client app)
#include <windows.h>
#include <winioctl.h>
#endif // __USE_NTOSKRNL__

#define DEVICE_NAME "YTDevice"
#define DEVICE_PATH L"\\Device\\"DEVICE_NAME
#define DOSDEVICE_PATH L"\\DosDevices\\"DEVICE_NAME
#define FILE_PATH "\\\\.\\"DEVICE_NAME

#define MY_IOCTL_PUSH \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

#define MY_IOCTL_POP \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

#endif // PUBLIC_H