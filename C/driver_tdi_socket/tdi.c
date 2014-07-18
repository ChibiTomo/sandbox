#include "tdi.h"

NTSTATUS tdi_initialize_handle(tdi_handle_t* tdi_handle, PIO_STACK_LOCATION pIoStackIrp) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("About to initialize tdi handle\n");

	HANDLE transport_handle;
	PFILE_OBJECT transport;
	status = tdi_open_transport_address(&transport_handle, &transport);
	if (status != STATUS_SUCCESS) {
		DEBUG("Cannot open transport\n");
		goto cleanup;
	}
	DEBUG("Transport opened\n");

	HANDLE connection_handle;
	PFILE_OBJECT connection;
	status = tdi_open_connection(&connection_handle, &connection);
	if (status != STATUS_SUCCESS) {
		DEBUG("Cannot open connection\n");
		goto cleanup;
	}
	DEBUG("Connection opened\n");

	status = tdi_associate_transport_connection(transport_handle, connection);
	if (status != STATUS_SUCCESS) {
		DEBUG("Cannot associate transport and connection\n");
		goto cleanup;
	}
	DEBUG("Transport and connection associated\n");

	tdi_handle->connection_handle = connection_handle;
	tdi_handle->transport_handle = transport_handle;
	tdi_handle->connection = connection;
	tdi_handle->transport = transport;

cleanup:
	if (status != STATUS_SUCCESS) {
		if (transport_handle) {
			tdi_close_handle(transport_handle, transport);
		}
		if (connection_handle) {
			tdi_close_handle(connection_handle, connection);
		}
	}
	return status;
}

NTSTATUS tdi_open_transport_address(PHANDLE phandle, PFILE_OBJECT* ppfile_object) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("open_transport_address\n");

	// Initialize extended attributes informations
	char dataBlob[sizeof(FILE_FULL_EA_INFORMATION) + TDI_TRANSPORT_ADDRESS_LENGTH + 300]; // +300 ???????
	RtlZeroMemory(dataBlob, sizeof(dataBlob));

	PFILE_FULL_EA_INFORMATION exAttrInfo = (PFILE_FULL_EA_INFORMATION) &dataBlob;
	RtlCopyMemory(&exAttrInfo->EaName, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH);

	exAttrInfo->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
	exAttrInfo->EaValueLength = TDI_TRANSPORT_ADDRESS_LENGTH + sizeof(TRANSPORT_ADDRESS) + sizeof(TDI_ADDRESS_IP);

	PTRANSPORT_ADDRESS transport_address =
			(PTRANSPORT_ADDRESS)(&exAttrInfo->EaName + TDI_TRANSPORT_ADDRESS_LENGTH + 1);
	transport_address->TAAddressCount = 1;
	transport_address->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
	transport_address->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);

	PTDI_ADDRESS_IP tdiAddressIp = (TDI_ADDRESS_IP*) &(transport_address->Address[0].Address);
	RtlZeroMemory(tdiAddressIp, sizeof(TDI_ADDRESS_IP));

	status = tdi_create_handle_file_object(phandle, ppfile_object, exAttrInfo, sizeof(dataBlob));

	return status;
}

NTSTATUS tdi_open_connection(PHANDLE phandle, PFILE_OBJECT* ppfile_object) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("open_connection\n");

	char dataBlob[sizeof(FILE_FULL_EA_INFORMATION) + TDI_TRANSPORT_ADDRESS_LENGTH + 300]; // +300 ???????
	RtlZeroMemory(dataBlob, sizeof(dataBlob));

	PFILE_FULL_EA_INFORMATION exAttrInfo = (PFILE_FULL_EA_INFORMATION) &dataBlob;
	RtlCopyMemory(&exAttrInfo->EaName, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH);

	exAttrInfo->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
	exAttrInfo->EaValueLength = TDI_CONNECTION_CONTEXT_LENGTH;

	status = tdi_create_handle_file_object(phandle, ppfile_object, exAttrInfo, sizeof(dataBlob));

	return status;
}

NTSTATUS tdi_create_handle_file_object(	PHANDLE phandle,
									PFILE_OBJECT* ppfile_object,
									PFILE_FULL_EA_INFORMATION exAttrInfo,
									int dataSize) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("create_handle_file_object\n");

	UNICODE_STRING tdiDeviceName;
	RtlInitUnicodeString(&tdiDeviceName, L"\\Device\\Tcp");
	OBJECT_ATTRIBUTES tdiDeviceNameAttr;
	InitializeObjectAttributes(	&tdiDeviceNameAttr,
								&tdiDeviceName,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								NULL,
								NULL);
	IO_STATUS_BLOCK IoStatusBlock;
	status = ZwCreateFile(	phandle,
							FILE_READ_EA | FILE_WRITE_EA,
							&tdiDeviceNameAttr,
							&IoStatusBlock,
							NULL,
							FILE_ATTRIBUTE_NORMAL,
							0,
							FILE_OPEN_IF,
							0,
							exAttrInfo,
							dataSize);
	if (status != STATUS_SUCCESS) {
		DEBUG("Cannot open file on TDI device\n");
		goto cleanup;
	}
	DEBUG("File opened on TDI device: 0x%08X\n", *phandle);

	status = ObReferenceObjectByHandle(	*phandle,
										GENERIC_READ | GENERIC_WRITE,
										NULL,
										KernelMode,
										(PVOID*) ppfile_object,
										NULL);
	if (status != STATUS_SUCCESS) {
		DEBUG("Cannot validate access to the file object\n");
		ZwClose(*phandle);
		goto cleanup;
	}
	DEBUG("Access to the file object validated\n");

cleanup:
	return status;
}

NTSTATUS tdi_associate_transport_connection(HANDLE transport_handle, PFILE_OBJECT connection) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("associate_transport_connection\n");

	PDEVICE_OBJECT tdi_device = IoGetRelatedDeviceObject(connection);
	IO_STATUS_BLOCK IoStatusBlock;
	PIRP pIrp = TdiBuildInternalDeviceControlIrp(TDI_ASSOCIATE_ADDRESS,
												tdi_device,
												connection,
												NULL,
												&IoStatusBlock);
	if (!pIrp) {
		DEBUG("Cannot create Irp for association\n");
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanup;
	}

	TdiBuildAssociateAddress(pIrp, tdi_device, connection, NULL, NULL, transport_handle);

	status = IoCallDriver(tdi_device, pIrp);
	if(status == STATUS_PENDING) {
		DEBUG("STATUS_PENDING. TODO: wait for completion...\n");
	}
	if (!NT_SUCCESS(status)) { // status can be Pending
		DEBUG("Cannot associate transport and connection: 0x%08X\n", status);
		goto cleanup;
	}
	DEBUG("Association done\n");

cleanup:
	return status;
}

NTSTATUS tdi_close_handle(HANDLE handle, PFILE_OBJECT file) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("close_tdi_handle\n");

	ObDereferenceObject(file);
	ZwClose(handle);

	return status;
}

void tdi_free_handle(tdi_handle_t* tdi_handle) {
	tdi_disassociate_transport_connection(tdi_handle->connection);
	tdi_close_handle(tdi_handle->connection_handle, tdi_handle->connection);
	tdi_close_handle(tdi_handle->transport_handle, tdi_handle->transport);

	tdi_handle->connection_handle = NULL;
	tdi_handle->transport_handle = NULL;

	tdi_handle->connection = NULL;
	tdi_handle->transport = NULL;
}

NTSTATUS tdi_disassociate_transport_connection(PFILE_OBJECT connection) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("disassociate_transport_connection\n");

	PDEVICE_OBJECT tdi_device = IoGetRelatedDeviceObject(connection);
	IO_STATUS_BLOCK IoStatusBlock;
	PIRP pIrp = TdiBuildInternalDeviceControlIrp(TDI_DISASSOCIATE_ADDRESS,
												tdi_device,
												connection,
												NULL,
												&IoStatusBlock);
	if (!pIrp) {
		DEBUG("Cannot create Irp for disassociation\n");
		goto cleanup;
	}

	TdiBuildDisassociateAddress(pIrp, tdi_device, connection, NULL, NULL);

	status = IoCallDriver(tdi_device, pIrp);
	if(status == STATUS_PENDING) {
		DEBUG("STATUS_PENDING. TODO: wait for completion...\n");
	}
	if (!NT_SUCCESS(status)) { // status can be Pending
		DEBUG("Cannot disassociate transport and connection: 0x%08X\n", status);
		goto cleanup;
	}
	DEBUG("Disassociation done\n");

cleanup:
	return status;
}