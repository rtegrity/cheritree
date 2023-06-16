# c18n-expt
Experimenting with compartmentalisation

~~~{.sh}
export LD_LIBRARY_PATH=. LD_64_LIBRARY_PATH=. LD_C18N_LIBRARY_PATH=.
~~~

## Exported variables

The size of the variable is encoded in the shared library and a suitable capability is created at load time.
As documented, there's currently no protection against vulnerabilities from modifying the shared libary.

~~~{.sh}
nm --print-size --extern-only lib1.so
~~~

## Capability Tree

Rework with libprocstat and libelf.


## Hybrid

Hybrid builds don't work with the compartmentalisation loader. They abort with an ENOEXEC error, which is reasonable.

## References

https://git.morello-project.org/morello/kernel/morello-ack/-/blob/morello/release-1.0/Documentation/arm64/morello.rst

https://github.com/sbaranga-arm/abi-aa/blob/f76dc02f1fe39ab667e342bc8d734aa4eb5b9e55/aapcs64-morello/aapcs64-morello.rst#parameter-passing

https://ftp.gnu.org/old-gnu/Manuals/gdb/html_node/gdb_toc.html

https://github.com/CTSRD-CHERI/cheribsd/blob/main/lib/libc/aarch64/gen/setjmp.S
