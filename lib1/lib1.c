/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif


int lib1_int = 30;
int lib1_int_bounds = 40;


void lib1_init()
{
}


/*
 *  Return the value of a const int.
 */
int lib1_access_int()
{
    return lib1_int;
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
    return atoi(s);
}