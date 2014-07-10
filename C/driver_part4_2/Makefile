CFLAGS = -Wall -Werror -I/mingw/include/ddk -std=gnu99 -Wno-multichar

all:
	#FIRST DRIVER
	gcc -c $(CFLAGS) -o first_driver.o first_driver.c
	gcc -Wl,--base-file,driver.base \
		-Wl,--subsystem,native \
		-Wl,--entry,_DriverEntry@8 \
		-nostartfiles -nostdlib \
		-o junk.tmp \
		first_driver.o \
		-lntoskrnl -lhal
	rm junk.tmp
	dlltool --dllname my_first_driver.sys \
		--base-file driver.base --output-exp first_driver.exp
	rm driver.base
	gcc -mdll \
		-Wl,--entry,_DriverEntry@8 \
		-Wl,--subsystem,native \
		-Wl,--image-base,0x10000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--stack,0x40000 \
		-Wl,first_driver.exp \
		-nostdlib -nostartfiles \
		-o my_first_driver.sys \
		first_driver.o \
		-lntoskrnl -lhal
	#SECOND DRIVER
	gcc -c $(CFLAGS) -o second_driver.o second_driver.c
	gcc -Wl,--base-file,driver.base \
		-Wl,--subsystem,native \
		-Wl,--entry,_DriverEntry@8 \
		-nostartfiles -nostdlib \
		-o junk.tmp \
		second_driver.o \
		-lntoskrnl -lhal
	rm junk.tmp
	dlltool --dllname my_second_driver.sys \
		--base-file driver.base --output-exp second_driver.exp
	rm driver.base
	gcc -mdll \
		-Wl,--entry,_DriverEntry@8 \
		-Wl,--subsystem,native \
		-Wl,--image-base,0x10000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--stack,0x40000 \
		-Wl,second_driver.exp \
		-nostdlib -nostartfiles \
		-o my_second_driver.sys \
		second_driver.o \
		-lntoskrnl -lhal
	#TESTER
	gcc -o tester tester.c

clean:
	rm -f *sys *o *base *tmp *pe *exp *def *dll.a *exe *i *s
