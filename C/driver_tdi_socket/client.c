#include "client.h"

int main() {
	int result = 0;
	HANDLE connection_handle = NULL;

	// Filling user info
	user_info_t usr_info;
	memset(&usr_info, 0, sizeof(user_info_t));
	snprintf(usr_info.nick, sizeof(usr_info.nick), "User");
	usr_info.host_address.address[0] = 127;
	usr_info.host_address.address[1] = 0;
	usr_info.host_address.address[2] = 0;
	usr_info.host_address.address[3] = 1;
	usr_info.host_address.port = 4000;

	DEBUG("**User info:\n");
	DEBUG("  Nick: %s\n", usr_info.nick);
	DEBUG("  Host: %d.%d.%d.%d\n", 	usr_info.host_address.address[0],
									usr_info.host_address.address[1],
									usr_info.host_address.address[2],
									usr_info.host_address.address[3]);
	DEBUG("  Port: %d\n", usr_info.host_address.port);
	DEBUG("\n");

	my_initialize_connection(&connection_handle);
	if (connection_handle == INVALID_HANDLE_VALUE) {
		DEBUG("Cannot initialize connection\n");
		result = 1;
		goto cleanup;
	}
	DEBUG("Connection initialized\n");
	DEBUG("Handle: 0x%08X\n", (unsigned int)connection_handle);

	network_address_t network_address;
	host2network(&(usr_info.host_address), &network_address);

	if (!my_connect(&connection_handle, &network_address)) {
		DEBUG("Not connected\n");
		result = 1;
		goto cleanup;
	}
	DEBUG("Connected\n");

cleanup:
	if (connection_handle) {
		my_close_connection(&connection_handle);
	}
	return result;
}

void my_initialize_connection(PHANDLE connection_handle) {
	DEBUG("About to initialize connection\n");
	*connection_handle = CreateFile(	FILE_PATH,
									GENERIC_READ | GENERIC_WRITE,
									0,
									NULL,
									OPEN_EXISTING,
									0,
									NULL);
	DEBUG("New handle: 0x%08X\n", (unsigned int) *connection_handle);
}

BOOLEAN my_connect(PHANDLE connection_handle, network_address_t* network_address) {
	DEBUG("About to connect\n");

	DWORD bytesReaded = 0;
	DeviceIoControl(*connection_handle,
					MY_IOCTL_TDI_CONNECT,
					&network_address,
					sizeof(network_address_t),
					NULL,
					0,
					&bytesReaded,
					NULL);
	return bytesReaded > 0;
}

void my_close_connection(PHANDLE connection_handle) {
	DEBUG("Close connection\n");
	CloseHandle(*connection_handle);
}

void host2network(host_address_t* host_address, network_address_t* network_address) {
	network_address->address = host_address->address[0];
	network_address->address = (network_address->address<<8) + host_address->address[1];
	network_address->address = (network_address->address<<8) + host_address->address[2];
	network_address->address = (network_address->address<<8) + host_address->address[3];

	// Translate port from host order to network order (big endian)
	network_address->port = host_address->port & 0x00FF;
	network_address->port = network_address->port<<8;
	network_address->port += (host_address->port>>8) & 0x00FF;
}