/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif
#include "cheritree.h"


extern void cheritree_saveregs(void **);
extern void _cheritree_find_capabilities(void **regs, int nregs);


void ___cheritree_find_capabilities()
{
    void *vec[64 * 1024];
    void *regs[33];

    cheritree_saveregs(regs);
    _cheritree_find_capabilities(regs, 33);
}

