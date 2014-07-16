#include <stdio.h>
//#include <stdlib.h>
#include "public.h"
#include "clientTDI.h"

int main() {
	int result = 0;
	CLIENT_CONNECT_INFO ClientConnectInfo;
	memset(&ClientConnectInfo, 0, sizeof(CLIENT_CONNECT_INFO));

	if (!Client_InitializeLightBulbs()) {
		printf("Initialization error\n");
		result = 1;
		goto cleanup;
	}

	if (!Client_GetHostInfoFromUser(&ClientConnectInfo)) {
		printf("Cannot get host info\n");
		result = 1;
		goto cleanup;
	}

	HANDLE hConnectSocket = Client_ConnectToHost(&ClientConnectInfo);
	if (hConnectSocket == INVALID_HANDLE_VALUE) {
		printf("Invalid socket\n");
		result = 1;
		goto cleanup;
	}

	Client_RunClient(hConnectSocket, ClientConnectInfo.szNick);
	Client_FreeLightBulbs();

cleanup:
	return result;
}

BOOL Client_InitializeLightBulbs() {
	BOOL bInitialized = TRUE;
	return bInitialized;
}

void Client_FreeLightBulbs() {
	/* No Implementation */
}

void Client_DisplayErrorMsg(CLIENT_ERROR uiMsg) {
	switch(uiMsg) {
		case CLIENT_LIGHTBULB_INIT_ERROR:
			printf("Chat Client - Cannot Initialize Light Bulbs!\n");
			break;

		case CLIENT_LIGHTBULB_BULB_ERROR:
			printf("Chat Client - Cannot Create Light Bulb!\n");
			break;

		case CLIENT_LIGHTBULB_CONNECT_ERROR:
			printf("Chat Server - Cannot Connect to Remote Server!\n");
			break;

		case CLIENT_LIGHTBULB_SEND_ERROR:
			printf("Chat Server - Light Bulb Error Data Not Sent!\n");
			break;

		case CLIENT_LIGHTBULB_SERVER_CLOSED_ERROR:
			printf("Chat Server - Server Closed!\n");
			break;

		default:
			printf("Chat Client - Runtime Error!\n");
	}
}

BOOL Client_GetHostInfoFromUser(PCLIENT_CONNECT_INFO pClientConnectInfo) {
	BOOL bGotUserInfo = TRUE;
//	char szHostName[101];

//	printf("Enter Your Name (Or A Nick Name, Max 11 Characters)\n");
//	fgets(pClientConnectInfo->szNick, 12, stdin);
//
//	if(strlen(pClientConnectInfo->szNick) == 0
//		|| (strlen(pClientConnectInfo->szNick) == 1 && pClientConnectInfo->szNick[0] == '\n'))
//	{
		strcpy(pClientConnectInfo->szNick, "User");
//	}
//
//	if(pClientConnectInfo->szNick[strlen(pClientConnectInfo->szNick) - 1] == '\n') {
//		pClientConnectInfo->szNick[strlen(pClientConnectInfo->szNick) - 1] = 0;
//	}
	printf("Username: %s\n", pClientConnectInfo->szNick);

	pClientConnectInfo->usPort = htons(4000);

//	printf("Enter Host IP Address like: 127.0.0.1\n");
//	fgets(szHostName, 100, stdin);
	printf("Host: 127.0.0.1\n");

	pClientConnectInfo->ulAddress = inet_addr("127.0.0.1");

	return bGotUserInfo;
}

HANDLE Client_ConnectToHost(PCLIENT_CONNECT_INFO pClientConnectInfo) {
	HANDLE hClientLightBulb = Network_CreateLightBulb();
	if(hClientLightBulb == INVALID_HANDLE_VALUE) {
		Client_DisplayErrorMsg(CLIENT_LIGHTBULB_BULB_ERROR);
		goto cleanup;
	}

	pClientConnectInfo->FirstAsyncRead.pszBuffer = pClientConnectInfo->szTempBuffer;
	Network_RecieveAsync(	hClientLightBulb,
							pClientConnectInfo->FirstAsyncRead.pszBuffer,
							sizeof(pClientConnectInfo->szTempBuffer) - 1,
							(LPOVERLAPPED) &pClientConnectInfo->FirstAsyncRead,
							Client_FirstReadCompletionRoutine);

	BOOL bConnected = Network_Connect(hClientLightBulb, pClientConnectInfo->ulAddress, pClientConnectInfo->usPort);

	if(bConnected == FALSE) {
		Client_DisplayErrorMsg(CLIENT_LIGHTBULB_CONNECT_ERROR);
		Network_CloseLightBulb(hClientLightBulb);
		hClientLightBulb = INVALID_HANDLE_VALUE;
		goto cleanup;
	}

	NETWORK_ASYNC_WRITE NetAsyncWrite;
	memset(&NetAsyncWrite, 0, sizeof(NETWORK_ASYNC_WRITE));

	Network_SendAsync(	hClientLightBulb,
						pClientConnectInfo->szNick,
						strlen(pClientConnectInfo->szNick),
						(LPOVERLAPPED) &NetAsyncWrite,
						Client_WriteCompletionRoutine);
	while(!NetAsyncWrite.bWriteComplete) {
		SleepEx(25, TRUE);
	}

cleanup:
	return hClientLightBulb;
}

void Client_RunClient(HANDLE hClientLightBulb, char *pszNick) {
	char szBuffer[1001];

	printf("Toby Opferman's Example Chat Client\n");
	printf("Enter a string to send or press ESC to exit\n");

	/*
	* Remember, we do no buffering in our driver.  However, we have enabled
	* asynchronous file operations which means we can make as many requests
	* as we want to the driver!  The actual joy of this is that we can now
	* make a ton of requests and hopefully not miss any data since we have
	* enough buffers in the waiting queue.  This helps to demonstrate how
	* to do asynchronous I/O.
	*/
	int nbrReceiveBuffer = 5;
	NETWORK_ASYNC_READ NetAsyncRead[nbrReceiveBuffer];
	memset(NetAsyncRead, 0, nbrReceiveBuffer);

	char szRecieveBuffer[nbrReceiveBuffer][300];
	memset(szRecieveBuffer, 0, nbrReceiveBuffer);
	for (int i = 0; i < nbrReceiveBuffer; i++) {
		memset(szRecieveBuffer[i], 0, 300);
	}

	for (int i = 0; i < nbrReceiveBuffer; i++) {
		NetAsyncRead[i].pszBuffer = szRecieveBuffer[i];
		NetAsyncRead[i].hClientLightBulb = hClientLightBulb;
		NetAsyncRead[i].uiSize = sizeof(szRecieveBuffer[i]);
	}

	for (int i = 0; i < nbrReceiveBuffer; i++) {
		Network_RecieveAsync(	hClientLightBulb,
								szRecieveBuffer[i],
								sizeof(szRecieveBuffer[i]) - 1,
								(LPOVERLAPPED) &NetAsyncRead[i],
								Client_ReadCompletionRoutine);
	}

	NETWORK_ASYNC_WRITE NetAsyncWrite;
	NetAsyncWrite.bWriteComplete = TRUE;

	BOOL bDoClientLoop = TRUE;
	while(bDoClientLoop) {
		SleepEx(25, TRUE);

		if(!NetAsyncWrite.bWriteComplete) {
			continue;
		}

		if(!kbhit()) {
			continue;
		}

		int cChar = getch();
		switch(cChar) {
			case 27 :
				bDoClientLoop = FALSE;
				break;

			default:
				printf("Type Text (Max 256 Characters):\n");
				printf("%c", cChar);

				sprintf(szBuffer, "%s: %c", pszNick, cChar);
				fgets(szBuffer + strlen(szBuffer), 256, stdin);

				memset(&NetAsyncWrite, 0, sizeof(NetAsyncWrite));

				Network_SendAsync(	hClientLightBulb,
									szBuffer,
									strlen(szBuffer),
									(LPOVERLAPPED) &NetAsyncWrite,
									Client_WriteCompletionRoutine);
				break;
		}
	}

	Network_CloseLightBulb(hClientLightBulb);
}

VOID CALLBACK Client_FirstReadCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
	PNETWORK_ASYNC_READ pNetworkAsyncRead = (PNETWORK_ASYNC_READ)lpOverlapped;

	printf(pNetworkAsyncRead->pszBuffer);
	printf("\n");
}

VOID CALLBACK Client_ReadCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
	PNETWORK_ASYNC_READ pNetworkAsyncRead = (PNETWORK_ASYNC_READ)lpOverlapped;

	printf(pNetworkAsyncRead->pszBuffer);
	printf("\n");

	memset(lpOverlapped, 0, sizeof(OVERLAPPED));
	memset(pNetworkAsyncRead->pszBuffer, 0, pNetworkAsyncRead->uiSize);

	Network_RecieveAsync(pNetworkAsyncRead->hClientLightBulb, pNetworkAsyncRead->pszBuffer, pNetworkAsyncRead->uiSize - 1, (LPOVERLAPPED)pNetworkAsyncRead, Client_ReadCompletionRoutine);
}

VOID CALLBACK Client_WriteCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
	PNETWORK_ASYNC_WRITE pNetworkAsyncWrite = (PNETWORK_ASYNC_WRITE)lpOverlapped;
	pNetworkAsyncWrite->bWriteComplete = TRUE;
}


HANDLE Network_CreateLightBulb() {
	HANDLE hLightBulb = INVALID_HANDLE_VALUE;
	hLightBulb = CreateFile("\\\\.\\LightBulb", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	return hLightBulb;
}

BOOL Network_Connect(HANDLE hLightBulb, ULONG ulAddress, USHORT usPort) {
	BOOL bConnected = FALSE;
	NETWORK_ADDRESS NetworkAddress;

	NetworkAddress.ulAddress = ulAddress;
	NetworkAddress.usPort = usPort;

	DWORD uiBytesReturned = 0;
	if(DeviceIoControl(hLightBulb, IOCTL_TDIEXAMPLE_CONNECT, &NetworkAddress, sizeof(NetworkAddress), NULL, 0, &uiBytesReturned, NULL) != FALSE) {
		bConnected = TRUE;
	}

	return bConnected;
}

void Network_Disconnect(HANDLE hLightBulb) {
	DWORD junk = 0;
	DeviceIoControl(hLightBulb, IOCTL_TDIEXAMPLE_DISCONNECT, NULL, 0, NULL, 0, &junk, NULL);
}

UINT Network_SendAsync(HANDLE hLightBulb, PVOID pData, UINT uiSize, LPOVERLAPPED lpOverLapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCallback) {
    UINT uiDataSent = 0;

    WriteFileEx(hLightBulb, pData, uiSize, lpOverLapped, lpCallback);

    return uiDataSent;
}

UINT Network_RecieveAsync(HANDLE hLightBulb, PVOID pData, UINT uiSize, LPOVERLAPPED lpOverLapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCallback) {
	UINT uiDataRead = 0;
	ReadFileEx(hLightBulb, pData, uiSize, lpOverLapped, lpCallback);
	return uiDataRead;
}

void Network_CloseLightBulb(HANDLE hLightBulb) {
	CloseHandle(hLightBulb);
}
