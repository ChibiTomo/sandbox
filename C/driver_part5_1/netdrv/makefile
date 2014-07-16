CFLAGS = -Wall -Werror -I/mingw/include/ddk -I../inc -std=gnu99 -Wno-multichar
DRIVER_SRC = entry.c functions.c HandleIrp.c kmem.c tdifuncs.c
DRIVER_OBJ = $(DRIVER_SRC:.c=.o)
HEADER = handleirp.h kmem.h tdiexample.h tdifuncs.h ../inc/drvmisc.h

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

.c.o: $(HEADER)
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o *.exe *.sys *.base *.tmp *.exp