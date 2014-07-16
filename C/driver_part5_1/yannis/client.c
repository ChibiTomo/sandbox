#include <windows.h>
#include <winsock.h>
#include <stdio.h>
#include <conio.h>

#define MY_HOST_NAME "127.0.0.1"
#define CLIENT_NICKNAME "Yannis"

BOOLEAN init_winsock();
SOCKET connect_to_host();
void run(SOCKET client_socket, char* nickname);
void free_winsock();

int main() {
	int result = 0;

	if (!init_winsock()) {
		printf("Cannot initialize winsock\n");
		result = 1;
		goto cleanup;
	}

	SOCKET client_socket = connect_to_host();
	if (client_socket == INVALID_SOCKET) {
		printf("Socket error\n");
		result = 1;
		goto cleanup;
	}

	run(client_socket, CLIENT_NICKNAME);

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

SOCKET connect_to_host() {
	SOCKET my_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (my_socket == INVALID_SOCKET) {
		printf("Cannot create socket\n");
		goto cleanup;
	}

	SOCKADDR_IN socket_host_addrs;
	socket_host_addrs.sin_family = PF_INET;
	socket_host_addrs.sin_port = htons(4000);
	socket_host_addrs.sin_addr.s_addr = inet_addr(MY_HOST_NAME);

	int retVal = connect(my_socket, (LPSOCKADDR) &socket_host_addrs, sizeof(SOCKADDR_IN));
	if (retVal == INVALID_SOCKET) {
		printf("Cannot connect to host: " MY_HOST_NAME "\n");
		closesocket(my_socket);
		my_socket = INVALID_SOCKET;
		goto cleanup;
	}

	send(my_socket, CLIENT_NICKNAME, strlen(CLIENT_NICKNAME), 0);

cleanup:
	return my_socket;
}

void run(SOCKET client_socket, char* nickname) {
	printf("Running...\n");

	struct timeval stTimeout;
	fd_set stReadFDS;
	char buf[1024];

	BOOLEAN do_loop = TRUE;
	while (do_loop) {
		FD_ZERO(&stReadFDS);
		FD_SET(client_socket, &stReadFDS);

		stTimeout.tv_sec = 0;
		stTimeout.tv_usec = 1;
		int retVal = select(0, &stReadFDS, NULL, NULL, &stTimeout);

		if(retVal == 0 || retVal == SOCKET_ERROR) {
			if(kbhit()) {
				int c = getch();
				if (c == 27) { // ESC pressed
					do_loop = FALSE;
				} else {
					printf("Type Text (Max 256 Characters):\n");
					printf("%c", c);

					sprintf(buf, "%s: %c", nickname, c);
					fgets(buf + strlen(buf), 256, stdin);

					retVal = send(client_socket, buf, strlen(buf), 0);

					if (retVal == SOCKET_ERROR) {
						printf("Send message error\n");
					}
				}
			}
		} else {
			if(FD_ISSET(client_socket, &stReadFDS)) {
				retVal = recv(client_socket, buf, 1000, 0);

				if(retVal == 0 || retVal == SOCKET_ERROR) {
					printf("Server were closed\n");
					do_loop = FALSE;
				} else {
					buf[retVal] = 0;
					printf("%s\n", buf);
				}
			}
		}
	}

	closesocket(client_socket);
}

void free_winsock() {
	WSACleanup();
}