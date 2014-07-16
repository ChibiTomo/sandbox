/**********************************************************************
 *
 *  Toby Opferman
 *
 *  User Mode Light Bulb Network Library
 *
 *     Hey, if someone can name their library "sockets", I can
 *     name mine "Light Blubs".
 *
 *  This library is used to communicate with our custom TDI Client Driver
 *
 *  This source code is licensed for educational purposes only.
 *
 *  Copyright (c) 2005, All Rights Resevered
 *
 **********************************************************************/


#include <windows.h>
#include <WINIOCTL.H>
#include <stdio.h>
#include <NETDRV.H>
#include <lightbulb.h>

#define DEBUG(format, ...) printf("[" __FILE__ ":%d] " format, __LINE__, ##__VA_ARGS__);

/*********************************************************
 *
 *   Network_CreateLightBulb
 *
 *      Creates a handle to a light bulb
 *
 *********************************************************/
HANDLE Network_CreateLightBulb(void) {
	HANDLE hLightBulb = INVALID_HANDLE_VALUE;
	DEBUG("About to create file\n");
	hLightBulb = CreateFile("\\\\.\\LightBulb", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

	return hLightBulb;
}



/*********************************************************
 *
 *   Network_Connect
 *
 *      Connects to a light bulb to a socket
 *
 *********************************************************/
BOOL Network_Connect(HANDLE hLightBulb, ULONG ulAddress, USHORT usPort) {
	BOOL bConnected = FALSE;
	NETWORK_ADDRESS NetworkAddress;
	DWORD uiBytesReturned;

	NetworkAddress.ulAddress = ulAddress;
	NetworkAddress.usPort    = usPort;

	DEBUG("About to connect: IOCTL connect\n");
	if(DeviceIoControl(hLightBulb, IOCTL_TDIEXAMPLE_CONNECT, &NetworkAddress, sizeof(NetworkAddress), NULL, 0, &uiBytesReturned, NULL) != FALSE) {
		bConnected = TRUE;
	}

	return bConnected;
}


/*********************************************************
 *
 *   Network_Disconnect
 *
 *      Removes a light bulb from a socket
 *
 *********************************************************/
void Network_Disconnect(HANDLE hLightBulb) {
	DEBUG("About to disconnect: IOCTL disconnect\n");
	DeviceIoControl(hLightBulb, IOCTL_TDIEXAMPLE_DISCONNECT, NULL, 0, NULL, 0, NULL, NULL);
}


/*********************************************************
 *
 *   Network_SendAsync
 *
 *      Sends data to a remote socket connection asynchronously
 *
 *********************************************************/
UINT Network_SendAsync(HANDLE hLightBulb, PVOID pData, UINT uiSize, LPOVERLAPPED lpOverLapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCallback) {
	UINT uiDataSent = 0;
	DEBUG("SendAsync: WriteFileEx\n");
	WriteFileEx(hLightBulb, pData, uiSize, lpOverLapped, lpCallback);

	return uiDataSent;
}




/*********************************************************
 *
 *   Network_RecieveAsync
 *
 *      Recieves data from a remote socket connection asynchronously
 *
 *********************************************************/
UINT Network_RecieveAsync(HANDLE hLightBulb, PVOID pData, UINT uiSize, LPOVERLAPPED lpOverLapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCallback) {
	UINT uiDataRead = 0;
	DEBUG("ReceiveAsync: ReadFileEx\n");
	ReadFileEx(hLightBulb, pData, uiSize, lpOverLapped, lpCallback);

	return uiDataRead;
}


/*********************************************************
 *
 *   Network_CloseLightBulb
 *
 *      Turns the lights off.
 *
 *********************************************************/
void Network_CloseLightBulb(HANDLE hLightBulb) {
	DEBUG("CloseLightBulb: CloseHandle\n");
	BOOL b = CloseHandle(hLightBulb);
	DEBUG("Close handle: %d\n", b);
}





