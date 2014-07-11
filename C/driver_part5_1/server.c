#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <string.h>

#define NAME_SIZE 1024
#define BUFFER_SIZE 1024

typedef struct _client_info_t {
	SOCKET socket;
	SOCKADDR_IN sockAddr;
	char nick[12];
	BOOL setNick;
	struct _client_info_t* next;
} client_info_t;

typedef struct {
	SOCKET socket;
	client_info_t* clt_info_list;
	fd_set stReadFDS;
	fd_set stXcptFDS;
	int dataSockets;
	BOOL is_running;
} server_info_t;

BOOLEAN init_winsock();
SOCKET create_server();
int display_local_ip(SOCKADDR_IN* socket_addrs);
void run(SOCKET server_socket);
BOOLEAN process_socket_wait(server_info_t* server_info);
void process_incomming_sockets(server_info_t* server_info);
void new_client_broadcast(SOCKET clt_socket, server_info_t* server_info);
void process_chat_broadcast(server_info_t* server_info);
void free_client_list(server_info_t* server_info);
void free_winsock();

int main() {
	int result = 0;

	if (!init_winsock()) {
		printf("Cannot initialize winsock\n");
		result = 1;
		goto cleanup;
	}

	SOCKET server_socket = create_server();
	if (server_socket == INVALID_SOCKET) {
		printf("Socket error\n");
		result = 1;
		goto cleanup;
	}

	run(server_socket);

cleanup:
	free_winsock();
	return result;
}

BOOLEAN init_winsock() {
	BOOLEAN result = TRUE;

	WSADATA WsaData;
	if(WSAStartup(MAKEWORD(1, 1), &WsaData) != 0) {
		result = FALSE;
	}

	return result;
}

SOCKET create_server() {
	UINT error = 0;

	SOCKET my_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (my_socket == INVALID_SOCKET) {
		printf("Cannot create socket\n");
		goto cleanup;
	}

	SOCKADDR_IN socket_addrs;
	socket_addrs.sin_family = PF_INET;
	socket_addrs.sin_port = htons(4000);

	error = bind(my_socket, (struct sockaddr *) &socket_addrs, sizeof(SOCKADDR_IN));
	if(error == INVALID_SOCKET) {
		printf("Binding error\n");
		goto cleanup;
	}

	error = listen(my_socket, 5);
	if (error != 0) {
		printf("Cannot listen\n");
		goto cleanup;
	}

	error = display_local_ip(&socket_addrs);
	if (error != 0) {
		printf("Cannot listen\n");
		goto cleanup;
	}

cleanup:
	if (error != 0 && my_socket != INVALID_SOCKET) {
		closesocket(my_socket);
		my_socket = INVALID_SOCKET;
	}
	return my_socket;
}

int display_local_ip(SOCKADDR_IN* socket_addrs) {
	int result = 0;

	if(!socket_addrs->sin_addr.s_addr) {
		printf("Ip not in the SOCKADDR. Looking for it...\n");

		char hostName[NAME_SIZE];
		if (gethostname(hostName, NAME_SIZE - 1) == SOCKET_ERROR) {
			printf("Cannot find hostname\n");
			result = 1;
			goto cleanup;
		}

		LPHOSTENT host = gethostbyname((LPSTR) hostName);
		if (!host) {
			printf("Cannot find hostname\n");
			result = 1;
			goto cleanup;
		}

		if (!host->h_addr) {
			printf("Cannot find host address\n");
			result = 1;
			goto cleanup;
		}

		socket_addrs->sin_addr.s_addr = *((long *) host->h_addr);
	}

	printf("Server IP = %i.%i.%i.%i\n", socket_addrs->sin_addr.s_addr & 0xFF,
										socket_addrs->sin_addr.s_addr>>8 & 0xFF,
										socket_addrs->sin_addr.s_addr>>16 & 0xFF,
										socket_addrs->sin_addr.s_addr>>24 & 0xFF);
	printf("Server port = %i\n", htons(socket_addrs->sin_port));

cleanup:
	return result;
}

void run(SOCKET server_socket) {
	server_info_t server_info;
	memset(&server_info, 0, sizeof(server_info_t));

	server_info.socket = server_socket;
	server_info.is_running = TRUE;

	printf("Server is running.\n");
	printf("Type 'q' to Quit\n");

	while (server_info.is_running) {
		if(process_socket_wait(&server_info)) {
			process_incomming_sockets(&server_info);
			process_chat_broadcast(&server_info);
		}
	}

	free_client_list(&server_info);
	closesocket(server_socket);
}

BOOLEAN process_socket_wait(server_info_t* server_info) {
	BOOLEAN something_to_process = FALSE;
	struct timeval stTimeout;
	client_info_t* clt_info;

	BOOLEAN data_to_process = FALSE;
	while(!data_to_process) {
		FD_ZERO(&server_info->stReadFDS);
		FD_ZERO(&server_info->stXcptFDS);
		FD_SET(server_info->socket, &server_info->stReadFDS);

		server_info->dataSockets = 0;

		clt_info = server_info->clt_info_list;
		while (clt_info) {
			FD_SET(clt_info->socket, &server_info->stReadFDS);
			clt_info = clt_info->next;
		}
		stTimeout.tv_sec = 1;
		stTimeout.tv_usec = 0;

		int nbr_socket_to_handle = select(0, &server_info->stReadFDS, NULL, &server_info->stXcptFDS, &stTimeout);

		if(nbr_socket_to_handle == 0 || nbr_socket_to_handle == SOCKET_ERROR) {
			if(kbhit()) {
				switch(getch()) {
					case 'q' :
					case 'Q' :
						data_to_process = TRUE;
						server_info->is_running = FALSE;
						break;
				}
			}
		} else {
			data_to_process = TRUE;
			something_to_process = TRUE;
			server_info->dataSockets = nbr_socket_to_handle;
		}
	}
	return something_to_process;
}

void process_incomming_sockets(server_info_t* server_info) {
	if(!FD_ISSET(server_info->socket, &server_info->stReadFDS)) {
		return;
	}

	server_info->dataSockets--;

	UINT length = sizeof(SOCKADDR_IN);
	SOCKADDR_IN newClientSockAddr = {0};
	SOCKET newClient = accept(server_info->socket, (struct sockaddr *) &newClientSockAddr, &length);
	if(newClient == INVALID_SOCKET) {
		printf("Error while accepting new client\n");
		return;
	}

	new_client_broadcast(newClient, server_info);
	//client_info_t* clt_info = create_new_client(newClient, &newClientSockAddr);
}

void new_client_broadcast(SOCKET clt_socket, server_info_t* server_info) {
	printf("Broadcasting the connection of a new client\n");
	char buffer[BUFFER_SIZE] = {0};
	client_info_t* client_info;


	client_info = server_info->clt_info_list;

	if (!client_info) {
		sprintf(buffer, "No body connected\n");
		goto cleanup;
	}

	sprintf(buffer, "People Connected: ");

	while(client_info) {
		sprintf(buffer, "%s %s", buffer, client_info->nick);
		client_info = client_info->next;
	}
	sprintf(buffer, "%s\n", buffer);

cleanup:
	send(clt_socket, buffer, strlen(buffer), 0);
}

void process_chat_broadcast(server_info_t* server_info) {
}

void free_client_list(server_info_t* server_info) {
	client_info_t* client_info = server_info->clt_info_list;

	client_info_t* next_clt = NULL;
	while(client_info) {
		next_clt = client_info->next;

		closesocket(client_info->socket);
		LocalFree(client_info);

		client_info = next_clt;
	}

	server_info->clt_info_list = NULL;
}

void free_winsock() {
	WSACleanup();
}