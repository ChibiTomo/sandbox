CFLAGS = -Wall -Werror -I/mingw/include/ddk -std=gnu99 -Wno-multichar
DRIVER_SRC = entry.c functions.c HandleIrp.c kmem.c tdifuncs.c
DRIVER_OBJ = $(DRIVER_SRC:.c=.o)

all: driver app

driver: $(DRIVER_OBJ)
	gcc -Wl,--base-file,driver.base \
		-Wl,--subsystem,native \
		-Wl,--entry,_DriverEntry@8 \
		-nostartfiles -nostdlib \
		-o junk.tmp \
		$(DRIVER_OBJ) \
		-lntoskrnl -lhal
	rm junk.tmp
	dlltool --dllname my_driver.sys \
		--base-file driver.base --output-exp driver.exp
	rm driver.base
	gcc -mdll \
		-Wl,--entry,_DriverEntry@8 \
		-Wl,--subsystem,native \
		-Wl,--image-base,0x10000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--stack,0x40000 \
		-Wl,driver.exp \
		-nostdlib -nostartfiles \
		-o my_driver.sys \
		$(DRIVER_OBJ) \
		-lntoskrnl -lhal

.c.o:
	gcc $(CFLAGS) -c $< -o $@

app:
	gcc $(CFLAGS) -o clientTDI clientTDI.c -lws2_32
	gcc $(CFLAGS) -o client client.c -lws2_32
	gcc $(CFLAGS) -o server server.c -lws2_32

clean:
	rm -rf *.o *.exp *.base *.exe *.sys