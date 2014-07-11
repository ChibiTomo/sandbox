CFLAGS = -Wall -Werror -I/mingw/include/ddk -std=gnu99 -Wno-multichar

all:
	gcc -c $(CFLAGS) -o driver.o driver.c
	gcc -Wl,--base-file,driver.base \
		-Wl,--subsystem,native \
		-Wl,--entry,_DriverEntry@8 \
		-nostartfiles -nostdlib \
		-o junk.tmp \
		driver.o \
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
		driver.o \
		-lntoskrnl -lhal
	#filter
	gcc -c $(CFLAGS) -o filter.o filter.c
	gcc -Wl,--base-file,filter.base \
		-Wl,--subsystem,native \
		-Wl,--entry,_DriverEntry@8 \
		-nostartfiles -nostdlib \
		-o junk.tmp \
		filter.o \
		-lntoskrnl -lhal
	rm junk.tmp
	dlltool --dllname my_filter_driver.sys \
		--base-file filter.base --output-exp filter.exp
	rm filter.base
	gcc -mdll \
		-Wl,--entry,_DriverEntry@8 \
		-Wl,--subsystem,native \
		-Wl,--image-base,0x10000 \
		-Wl,--file-alignment,0x1000 \
		-Wl,--section-alignment,0x1000 \
		-Wl,--stack,0x40000 \
		-Wl,filter.exp \
		-nostdlib -nostartfiles \
		-o my_filter_driver.sys \
		filter.o \
		-lntoskrnl -lhal
	gcc -o tester tester.c

clean:
	rm -f *sys *o *base *tmp *pe *exp *def *dll.a *exe *i *s
