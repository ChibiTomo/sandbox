#ifndef CLIENT_H
#define CLIENT_H

#include "public.h"

typedef struct {
	unsigned char address[4];
	unsigned short port;
} host_address_t;

typedef struct {
	char nick[NICKNAME_SIZE];
	host_address_t host_address;
} user_info_t;

void my_initialize_connection(PHANDLE connection_handle);
BOOLEAN my_connect(PHANDLE connection_handle, network_address_t* network_address);
void my_close_connection(PHANDLE connection_handle);

void host2network(host_address_t* host_address, network_address_t* network_address);

#endif // CLIENT_H