/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <assert.h>
#include <stdio.h>
#include "lib1.h"
#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif
#include <cheritree.h>


void lib2_init()
{
    cheritree_init();
}


/*
 *  Return the value of a const int with a cross compartment
 *  call.
 */
int lib2_access_int()
{
    return lib1_access_int();
}


/*
 *  Return the value of a string argument and validate that
 *  the bounds have been preserved with a cross compartment call.
 */
int lib2_check_string_arg(const char *s, size_t len)
{
#ifdef __CHERI_PURE_CAPABILITY__
    assert(cheri_length_get(s) == len);
#endif
    return lib1_check_string_arg(s, len);
}
