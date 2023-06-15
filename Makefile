CFLAGS=-march=morello -mabi=purecap -g -Ilib1 -Ilib2
# CFLAGS=-march=morello -mabi=aapcs -g -Ilib1 -Ilib2
C18NFLAGS=-Wl,--dynamic-linker=/libexec/ld-elf-c18n.so.1

rebuild: clean all

all:	shared comp

shared:	main.c lib1.so lib2.so
	cc $(CFLAGS) main.c vm.c saveregs.S -o shared lib1.so lib2.so

comp:	main.c lib1.so lib2.so
	cc $(CFLAGS) $(C18NFLAGS) vm.c saveregs.S main.c -o comp lib1.so lib2.so

lib1.so: lib1/lib1.c
	cc -fPIC -shared $(CFLAGS) -Wl,--version-script=lib1/lib1.map lib1/lib1.c -o lib1.so

lib2.so: lib2/lib2.c
	cc -fPIC -shared $(CFLAGS) -Wl,--version-script=lib2/lib2.map lib2/lib2.c -o lib2.so

clean:
	rm -f lib1.so lib2.so shared comp