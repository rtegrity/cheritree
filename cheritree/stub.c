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
extern void _cheritree_find_capabilities(void **regs, int nregs, void *ddc, void *pcc);


void cheritree_find_capabilities()
{
    void *regs[32];

    cheritree_saveregs(regs);
    
    cheritree_init();
    _cheritree_find_capabilities(regs, 32, cheri_ddc_get(), cheri_pcc_get());
}
