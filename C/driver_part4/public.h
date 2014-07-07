#define DEVICE_NAME "YTDevice"

#define FILE_PATH "\\\\.\\"DEVICE_NAME"\\tmp_file"

#define DEVICE_PATH L"\\Device\\" DEVICE_NAME
#define DOSDEVICE_PATH L"\\DosDevice\\" DEVICE_NAME

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define MY_IOCTL_DIRECT_IN_IO \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_IN_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)

#define MY_IOCTL_DIRECT_OUT_IO \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_OUT_DIRECT, FILE_READ_DATA | FILE_WRITE_DATA)

#define MY_IOCTL_BUFFERED_IO \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

//#define IOCTL_EXAMPLE_SAMPLE_NEITHER_IO \
//	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
