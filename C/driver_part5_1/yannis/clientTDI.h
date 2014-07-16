#ifndef __CLIENTTDI_H__
#define __CLIENTTDI_H__

typedef struct _network_async_read {
	OVERLAPPED ov;
	char *pszBuffer;
	UINT uiSize;
	HANDLE hClientLightBulb;
} NETWORK_ASYNC_READ, *PNETWORK_ASYNC_READ;

typedef struct _network_async_write {
	OVERLAPPED ov;
	BOOL bWriteComplete;
} NETWORK_ASYNC_WRITE, *PNETWORK_ASYNC_WRITE;

typedef struct _client_connect_info {
	char szNick[12];
	char szTempBuffer[100];
	ULONG ulAddress;
	USHORT usPort;
	NETWORK_ASYNC_READ FirstAsyncRead;
} CLIENT_CONNECT_INFO, *PCLIENT_CONNECT_INFO;

typedef enum {
	CLIENT_LIGHTBULB_INIT_ERROR = 1,
	CLIENT_LIGHTBULB_BULB_ERROR,
	CLIENT_LIGHTBULB_CONNECT_ERROR,
	CLIENT_LIGHTBULB_SEND_ERROR,
	CLIENT_LIGHTBULB_SERVER_CLOSED_ERROR
} CLIENT_ERROR;

BOOL Client_InitializeLightBulbs();
void Client_FreeLightBulbs();
BOOL Client_GetHostInfoFromUser(PCLIENT_CONNECT_INFO pClientConnectInfo);
HANDLE Client_ConnectToHost(PCLIENT_CONNECT_INFO pClientConnectInfo);
void Client_RunClient(HANDLE hClientLightBulb, char *pszNick);
void Client_DisplayErrorMsg(CLIENT_ERROR uiMsg);
VOID CALLBACK Client_FirstReadCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
VOID CALLBACK Client_ReadCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
VOID CALLBACK Client_WriteCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

HANDLE Network_CreateLightBulb();
BOOL Network_Connect(HANDLE hLightBulb, ULONG ulAddress, USHORT usPort);
void Network_Disconnect(HANDLE hLightBulb);
UINT Network_SendAsync(HANDLE hLightBulb, PVOID pData, UINT uiSize, LPOVERLAPPED lpOverLapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCallback);
UINT Network_RecieveAsync(HANDLE hLightBulb, PVOID pData, UINT uiSize, LPOVERLAPPED lpOverLapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCallback);
void Network_CloseLightBulb(HANDLE hLightBulb);

#endif // __CLIENTTDI_H__