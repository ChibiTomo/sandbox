all:
	gcc -c -I/mingw/include/ddk -o driver.o driver.c
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

clean:
	rm -f *sys *o *base *tmp *pe *exp *def *dll.a


#dll_import:
#	echo NAME ntoskrnl > ntoskrnl.def
#	echo EXPORTS >> ntoskrnl.def
#	echo DbgPrint >> ntoskrnl.def
#	pe_reader "C:/WINDOWS/system32/ntoskrnl.exe" | grep "ExportSymbol" | perl -pe "s/^ExportSymbol: (.*) \\(0x.*$$/\\1/g" >> ntoskrnl.def
#	dlltool --add-stdcall-underscore -d ntoskrnl.def -l ntoskrnl.dll.a