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
#include "lib1.h"
#include "lib2.h"
#include "vm.h"

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif


/*
 *  Check that an external int can be accessed and that
 *  the bounds are preserved.
 */
void access_int()
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
void access_int_bounds()
{
    extern long long lib1_int_bounds;
#ifdef __CHERI_PURE_CAPABILITY__
    assert(cheri_length_get(&lib1_int_bounds) == sizeof(lib1_int));
#endif
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


int main (int argc, char **argv)
{
    int dummy = 0;
    char *cp;

    cp = malloc(1);
    add_mapping_name(&main, &dummy, cp);
    free(cp);

    lib1_init();
    lib2_init();

    access_int();
    access_int_bounds();

    check_string_arg();

  find_memory_references();

//    print_mappings();
    return 0;
}
