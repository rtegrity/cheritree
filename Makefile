INCLUDES=-Ilib1 -Ilib2 -Icheritree -I../cheritree
CFLAGS=-march=morello -mabi=purecap -g $(INCLUDES)
# CFLAGS=-march=morello -mabi=aapcs -g $(INCLUDES)
C18NFLAGS=-Wl,--dynamic-linker=/libexec/ld-elf-c18n.so.1

rebuild: clean all

all:	shared comp

shared:	main.c lib1.so lib2.so cheritree.so cheritreestub.a
	cc $(CFLAGS) main.c cheritreestub.a -o shared lib1.so lib2.so cheritree.so

comp:	main.c lib1.so lib2.so cheritree.so cheritreestub.a
	cc $(CFLAGS) $(C18NFLAGS) main.c cheritreestub.a -o comp lib1.so lib2.so cheritree.so

cheritree.so: cheritree/cheritree.c cheritree/mapping.c cheritree/symbol.c \
		cheritree/util.c cheritree/stubs.S cheritreestub.a
	cc -fPIC -shared $(CFLAGS) -Wl,--version-script=cheritree/cheritree.map cheritree/cheritree.c \
		cheritree/mapping.c cheritree/symbol.c cheritree/util.c stubs.o -o cheritree.so

cheritreestub.a: cheritree/stubs.S
	cc -fPIC -c cheritree/stubs.S
	ar -rc cheritreestub.a stubs.o

lib1.so: lib1/lib1.c cheritreestub.a
	cc -fPIC -shared $(CFLAGS) -Wl,--version-script=lib1/lib1.map lib1/lib1.c cheritreestub.a -o lib1.so

lib2.so: lib2/lib2.c cheritreestub.a
	cc -fPIC -shared $(CFLAGS) -Wl,--version-script=lib2/lib2.map lib2/lib2.c cheritreestub.a -o lib2.so

clean:
	rm -f lib1.so lib2.so cheritree.so cheritreestub.a stubs.o shared comp