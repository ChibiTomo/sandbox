#ifndef __PUBLIC_H__
#define __PUBLIC_H__

#ifndef __USE_NTOSKRNL__ // Not inside the driver (ie: in client app)
#include <windows.h>
#include <winsock.h>
#include <winioctl.h>
#include <conio.h>
#endif // __USE_NTOSKRNL__

typedef struct _NETWORK_ADDRESS {
	ULONG ulAddress;
	USHORT usPort;
} NETWORK_ADDRESS, *PNETWORK_ADDRESS;

/* This IOCTL tells the driver to make a connection
 *  INPUT: NETWORK_ADDRESS
 *  OUTPUT: None
 */
#define IOCTL_TDIEXAMPLE_CONNECT \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

/* This IOCTL tells the driver to disconnect
 *  INPUT: None
 *  OUTPUT: None
 */
#define IOCTL_TDIEXAMPLE_DISCONNECT \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

#endif // __PUBLIC_H__
