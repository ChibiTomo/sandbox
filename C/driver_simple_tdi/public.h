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

#define DEVICE_NAME "YTDevice"
#define DEVICE_PATH L"\\Device\\" DEVICE_NAME
#define DOSDEVICE_PATH L"\\DosDevices\\" DEVICE_NAME
#define FILE_PATH "\\\\.\\" DEVICE_NAME

#endif // PUBLIC_H