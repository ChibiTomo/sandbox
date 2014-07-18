#include "driver.h"

NTSTATUS STDCALL DriverEntry(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath) {
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_OBJECT deviceObject = NULL;

	DEBUG("Load\n");

	driverObject->DriverUnload = my_unload;

	UNICODE_STRING deviceName;
	RtlInitUnicodeString(&deviceName, DEVICE_PATH);

	status = IoCreateDevice(driverObject,
							sizeof(extension_t),
							&deviceName,
							FILE_DEVICE_UNKNOWN,
							FILE_DEVICE_SECURE_OPEN,
							FALSE,
							&deviceObject);
	if (status != STATUS_SUCCESS) {
		DEBUG("Cannot create device\n");
		goto cleanup;
	}

	extension_t* extension = (extension_t*) deviceObject->DeviceExtension;

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		driverObject->MajorFunction[i] = my_ignored_function;
	}
	driverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = my_internal_ioctl;

	UNICODE_STRING filteredDeviceName;
	RtlInitUnicodeString(&filteredDeviceName , L"\\Device\\Tcp");

	status = IoAttachDevice(deviceObject, &filteredDeviceName, &extension->topOfDeviceStack);
	if (status != STATUS_SUCCESS) {
		DEBUG("Cannot attach to %wZ\n", &filteredDeviceName);
		goto cleanup;
	}
	DEBUG("Successfully attached to %wZ\n", &filteredDeviceName);

	deviceObject->Flags |= DO_BUFFERED_IO;
	deviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, DOSDEVICE_PATH);
	IoCreateSymbolicLink(&dosDeviceName, &deviceName);

cleanup:
	if (status != STATUS_SUCCESS) {
		if (deviceObject) {
			IoDeleteDevice(deviceObject);
		}
	}
	return status;
}

VOID STDCALL my_unload(PDRIVER_OBJECT driverObject) {
	DEBUG("Unload\n");

	extension_t* extension = (extension_t*) driverObject->DeviceObject->DeviceExtension;
	IoDetachDevice(extension->topOfDeviceStack);

	UNICODE_STRING dosDeviceName;
	RtlInitUnicodeString(&dosDeviceName, DOSDEVICE_PATH);
	IoDeleteSymbolicLink(&dosDeviceName);

	IoDeleteDevice(driverObject->DeviceObject);
}

NTSTATUS STDCALL my_ignored_function(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_NOT_SUPPORTED;

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DEBUG("Ignored function called without Irp location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}
	//DEBUG("Ignored function: 0x%02X\n", pIoStackIrp->MajorFunction);

cleanup:
	IoSkipCurrentIrpStackLocation(Irp);

	extension_t* extension = (extension_t*) deviceObject->DeviceExtension;
	status = IoCallDriver (extension->topOfDeviceStack, Irp);

	return status;
}

NTSTATUS STDCALL my_internal_ioctl(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN passDownIrp = TRUE;

	DEBUG("my_internal_ioctl\n");

	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);
	if (!pIoStackIrp) {
		DEBUG("No Irp location\n");
		status = STATUS_UNSUCCESSFUL;
		goto cleanup;
	}

	switch (pIoStackIrp->MinorFunction) {
		case TDI_CONNECT:
			status = process_tdi_connect(deviceObject, Irp);
			passDownIrp = FALSE;
			break;

		case TDI_SEND:
			DEBUG("TDI_SEND packet TCP, size: %d\n", pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength);
			break;

		default:
			DEBUG("TDI function called: 0x%08X\n", pIoStackIrp->MinorFunction);
	}

cleanup:
	if (passDownIrp) {
		IoSkipCurrentIrpStackLocation(Irp);
		extension_t* extension = (extension_t*) deviceObject->DeviceExtension;
		status = IoCallDriver(extension->topOfDeviceStack, Irp );
	}
	return status;
}

NTSTATUS process_tdi_connect(PDEVICE_OBJECT deviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_SUCCESS;

	DEBUG("TDI_CONNECT via TCP\n");

	// If we are here, there is a stack location
	PIO_STACK_LOCATION pIoStackIrp = IoGetCurrentIrpStackLocation(Irp);

	PTDI_REQUEST_KERNEL_CONNECT TDI_connectRequest =
			(PTDI_REQUEST_KERNEL_CONNECT) &(pIoStackIrp->Parameters);
	PTA_ADDRESS TA_Address_data =
			((PTRANSPORT_ADDRESS) TDI_connectRequest->RequestConnectionInformation->RemoteAddress)->Address;
	PTDI_ADDRESS_IP TDI_data = (PTDI_ADDRESS_IP) TA_Address_data->Address;

	DEBUG("Address type: %d\n", TA_Address_data->AddressType);
	DEBUG("Address length: %d\n", TA_Address_data->AddressLength);

	network_address_t network_address;
	convert_to_network_address(&network_address, TDI_data);

	DEBUG("TCP address: %i.%i.%i.%i:%i\n", 	network_address.address[0],
											network_address.address[1],
											network_address.address[2],
											network_address.address[3],
											network_address.port);

	if (network_address.address[0] == 192
			&& network_address.address[1] == 168
			&& network_address.address[2] == 154
			&& network_address.address[3] == 129) { // Virtual machine IP

		DEBUG("Redirecting\n");
		network_address.address[0] = 192; // Real machine IP
		network_address.address[1] = 168;
		network_address.address[2] = 1;
		network_address.address[3] = 37;
		set_network_address(TDI_data, &network_address);

		convert_to_network_address(&network_address, TDI_data);
		DEBUG("new TCP address: %i.%i.%i.%i:%i\n", 	network_address.address[0],
													network_address.address[1],
													network_address.address[2],
													network_address.address[3],
													network_address.port);
	}

	extension_t* extension = (extension_t*) deviceObject->DeviceExtension;
	if (IoForwardIrpSynchronously(extension->topOfDeviceStack, Irp)) {
		status = Irp->IoStatus.Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		DEBUG("Request status: 0x%08X\n", status);
		goto cleanup;
	}

	IoSkipCurrentIrpStackLocation(Irp);
	status = IoCallDriver(extension->topOfDeviceStack, Irp );
cleanup:
	return status;
}

void set_network_address(PTDI_ADDRESS_IP address_ip, network_address_t* network_address) {
	address_ip->in_addr = network_address->address[3];
	address_ip->in_addr = (address_ip->in_addr<<8) + network_address->address[2];
	address_ip->in_addr = (address_ip->in_addr<<8) + network_address->address[1];
	address_ip->in_addr = (address_ip->in_addr<<8) + network_address->address[0];
}

void convert_to_network_address(network_address_t* network_address, PTDI_ADDRESS_IP address_ip) {
	network_address->address[0] = address_ip->in_addr & 0x000000FF;
	network_address->address[1] = (address_ip->in_addr>>8) & 0x000000FF,
	network_address->address[2] = (address_ip->in_addr>>16) & 0x000000FF,
	network_address->address[3] = (address_ip->in_addr>>24) & 0x000000FF,

	// Translate port from network order to host order (little endian)
	network_address->port = address_ip->sin_port & 0x00FF;
	network_address->port = network_address->port<<8;
	network_address->port += address_ip->sin_port>>8 & 0x00FF;
}