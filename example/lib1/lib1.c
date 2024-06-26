/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif
#include <cheritree.h>


typedef int (*int_func_t)();

int lib1_access_int();


int lib1_int = 30;
int lib1_int_bounds = 40;


void lib1_init()
{
    cheritree_init();

    // Force symbol binding for local access
    lib1_access_int();
}


/*
 *  Return the value of a const int.
 */
int lib1_access_int()
{
    return lib1_int;
}


/*
 *  Return the value of an int using a local call.
 */
int lib1_local_access_int()
{
//  cheritree_print_capabilities();
    return lib1_access_int();
}


/*
 *  Return the value of an int using a function call.
 */
int lib1_function_access_int()
{
    return lib1_access_int();
}


/*
 *  Return a function pointer to access the value of an int
 */
int_func_t lib1_access_int_function()
{
    return lib1_function_access_int;
}


/*
 *  Return the value of a string argument and validate that
 *  the bounds have been preserved.
 */
int lib1_check_string_arg(const char *s, size_t len)
{
#ifdef __CHERI_PURE_CAPABILITY__
    assert(cheri_length_get(s) == len);
#endif
//  cheritree_print_capabilities();
    return atoi(s);
}