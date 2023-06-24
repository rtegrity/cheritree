/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include "cheritree.h"


extern void cheritree_saveregs(void **);
extern void _cheritree_find_capabilities(void **regs, int nregs);


void cheritree_find_capabilities()
{
    void *regs[32];

    cheritree_saveregs(regs);
    
    cheritree_init();
    _cheritree_find_capabilities(regs, 32);
}
