INCLUDES=-Ilib1 -Ilib2 -Icheritree -I../cheritree
CFLAGS=-march=morello -mabi=purecap -g $(INCLUDES)
# CFLAGS=-march=morello -mabi=aapcs -g $(INCLUDES)
C18NFLAGS=-Wl,--dynamic-linker=/libexec/ld-elf-c18n.so.1

rebuild: clean all

all:	shared comp

shared:	main.c lib1.so lib2.so cheritree.so
	cc $(CFLAGS) main.c -o shared lib1.so lib2.so cheritree.so

comp:	main.c lib1.so lib2.so cheritree.so
	cc $(CFLAGS) $(C18NFLAGS) main.c -o comp lib1.so lib2.so cheritree.so

cheritree.so: cheritree/cheritree.c cheritree/saveregs.S cheritree/mapping.c cheritree/symbol.c cheritree/module.c \
		cheritree/util.c
	cc -fPIC -shared $(CFLAGS) -Wl,--version-script=cheritree/cheritree.map cheritree/cheritree.c cheritree/saveregs.S \
		cheritree/mapping.c cheritree/symbol.c cheritree/module.c cheritree/util.c -o cheritree.so -lprocstat

lib1.so: lib1/lib1.c
	cc -fPIC -shared $(CFLAGS) -Wl,--version-script=lib1/lib1.map lib1/lib1.c -o lib1.so

lib2.so: lib2/lib2.c
	cc -fPIC -shared $(CFLAGS) -Wl,--version-script=lib2/lib2.map lib2/lib2.c -o lib2.so

clean:
	rm -f lib1.so lib2.so cheritree.so shared comp