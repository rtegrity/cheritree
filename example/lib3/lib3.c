/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif
#include <cheritree.h>


extern void register_library(int *v);

int lib3_int = 50;
__thread pid_t local_pid;


void lib3_init()
{
    cheritree_init();
}


static void
__attribute__((constructor))
lib3_constructor()
{
    local_pid = getpid();
    register_library(&lib3_int);
}


/*
 *  Return the value of a const int.
 */
int lib3_access_int()
{
    return lib3_int;
}
