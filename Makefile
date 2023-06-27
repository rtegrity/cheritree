INCLUDES=-Iexample/lib1 -Iexample/lib2 -Isrc -I../src
CFLAGS=-march=morello -mabi=purecap -g $(INCLUDES)
# CFLAGS=-march=morello -mabi=aapcs -g $(INCLUDES)
C18NFLAGS=-Wl,--dynamic-linker=/libexec/ld-elf-c18n.so.1

rebuild: clean all

all:	shared comp

shared:	example/main.c lib1.so lib2.so cheritree.so cheritreestub.a
	cc $(CFLAGS) example/main.c cheritreestub.a -o shared lib1.so lib2.so cheritree.so

comp:	example/main.c lib1.so lib2.so cheritree.so cheritreestub.a
	cc $(CFLAGS) $(C18NFLAGS) example/main.c cheritreestub.a -o comp lib1.so lib2.so cheritree.so

cheritree.so: src/cheritree.c src/mapping.c src/symbol.c \
		src/util.c src/stubs.S cheritreestub.a
	cc -fPIC -shared $(CFLAGS) -Wl,--version-script=src/cheritree.map src/cheritree.c \
		src/mapping.c src/symbol.c src/util.c stubs.o -o cheritree.so

cheritreestub.a: src/stubs.S
	cc -fPIC -c src/stubs.S
	ar -rc cheritreestub.a stubs.o

lib1.so: example/lib1/lib1.c cheritreestub.a
	cc -fPIC -shared $(CFLAGS) -Wl,--version-script=example/lib1/lib1.map example/lib1/lib1.c cheritreestub.a -o lib1.so

lib2.so: example/lib2/lib2.c cheritreestub.a
	cc -fPIC -shared $(CFLAGS) -Wl,--version-script=example/lib2/lib2.map example/lib2/lib2.c cheritreestub.a -o lib2.so

clean:
	rm -f lib1.so lib2.so cheritree.so cheritreestub.a stubs.o shared comp