/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <dlfcn.h>
#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif
#include <cheritree.h>
#include "lib1.h"
#include "lib2.h"


/*
 *  Check that an external int can be accessed and that
 *  the bounds are preserved.
 */
void check_access_int()
{
#ifdef __CHERI_PURE_CAPABILITY__
    assert(cheri_length_get(&lib1_int) == sizeof(lib1_int));
#endif
    assert(lib1_access_int() == lib1_int);
    assert(lib2_access_int() == lib1_int);
}


/*
 *  Check that the bounds of an external integer are
 *  based on the definition rather than the reference.
 */
void check_access_int_bounds()
{
    extern long long lib1_int_bounds;
#ifdef __CHERI_PURE_CAPABILITY__
    assert(cheri_length_get(&lib1_int_bounds) == sizeof(lib1_int));
#endif
}


/*
 *  Check that an external int can be accessed through a
 *  local call inside the library.
 */
void check_local_access_int()
{
    assert(lib1_local_access_int() == lib1_int);
}


/*
 *  Check that a function pointer from a library will
 *  correctly access the int.
 */
void check_access_int_function()
{
    int_func_t fn = lib1_access_int_function();
    assert(fn() == lib1_int);
}


/*
 *  Check that a string argument can be assessed.
 */
void check_string_arg()
{
    static const char s[] = "103";
    size_t len = sizeof(s);

#ifdef __CHERI_PURE_CAPABILITY__
    assert(cheri_length_get(s) == len);
#endif
    assert(lib1_check_string_arg(s, len) == atoi(s));
    assert(lib2_check_string_arg(s, len) == atoi(s));
}


/*
 *  Check dynamic loading.
 */
static int library_int;

void register_library(int *ptr)
{
    library_int = *ptr;
}

void check_dynamic_loading()
{
    void *dlhandle = dlopen("lib3.so", RTLD_LAZY);
    typedef int (*intfn_t)();
    typedef void (*init_t)();
    intfn_t getint;
    init_t init;
    int *intptr;

    assert(dlhandle != NULL);

    init = (init_t)dlsym(dlhandle, "lib3_init");
    assert(init != NULL);
    init();

    getint = (intfn_t)dlsym(dlhandle, "lib3_access_int");
    assert(getint != NULL);
    assert(getint() == library_int);

    intptr = (int *)dlsym(dlhandle, "lib3_int");
    assert(intptr != NULL);
    assert(*intptr == library_int);
}


int main (int argc, char **argv)
{
    cheritree_init();

    lib1_init();
    lib2_init();

    check_access_int();
    check_access_int_bounds();
    check_local_access_int();
    check_access_int_function();

    check_string_arg();
    check_dynamic_loading();

//  cheritree_print_mappings();
//  cheritree_print_capabilities();

    return 0;
}
