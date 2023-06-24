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
#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif
#include "mapping.h"
#include "symbol.h"


extern int saveregs(void **);


static void *printed[8192];
static int nprinted;


void _cheritree_init(void *function, void *stack)
{
    mapping_t *functionmap = cheritree_resolve_mapping((addr_t)function);
    mapping_t *stackmap = cheritree_resolve_mapping((addr_t)stack);
    cheritree_set_mapping_name(stackmap, functionmap, "stack");
}


static void print_address(void *vaddr, char *name, int depth)
{
    addr_t addr = (addr_t)cheri_address_get(vaddr);
    mapping_t *mapping = cheritree_resolve_mapping(addr);
    symbol_t *symbol;
    addr_t offset;
    int i;

    for (i = 0; i < depth; i++) putc(' ', stdout);

    if (!mapping || !*getname(mapping)) {
        printf("%s %#p\n", name, vaddr);
        return;
    }

    symbol = cheritree_find_symbol(getpath(mapping), getbase(mapping), addr);
    offset = addr - (addr_t)getbase(mapping);

    printf("%s %#p    ", name, vaddr);

    if (!*getpath(mapping) || !symbol || !*getname(symbol)) {
        printf("%s+%" PRIxADDR "\n", getname(mapping), offset);
        return;
    }

    offset -= symbol->value;

    if (offset)
        printf("%s!%s+%" PRIxADDR "\n", getname(mapping), getname(symbol), offset);

    else printf("%s!%s\n", getname(mapping), getname(symbol));
}


static int get_pointer_range(void *vaddr, void ***pstart, uintptr_t *pend)
{
    uintptr_t end;
    void **ptr;

    if (cheri_is_sentry(vaddr)) return 0;

    ptr = (void **)((char *)vaddr - ((addr_t)vaddr -
        cheri_align_up(cheri_base_get(vaddr), sizeof(void *))));

    end = cheri_align_down(cheri_base_get(ptr) +
         cheri_length_get(ptr), sizeof(void *));

    if (!cheri_is_valid(ptr)) return 0;
    if (cheri_base_get(ptr) > cheri_address_get(ptr)) return 0;

    if (cheri_address_get(ptr) >= cheri_base_get(ptr) + cheri_length_get(ptr))
        return 0;

    *pstart = ptr;
    *pend = end;
    return 1;
}


static int is_printed(void *addr)
{
    int i = 0;

    for (; i < nprinted; i++)
        if (printed[i] == addr || 
                (cheri_base_get(printed[i]) <= cheri_address_get(addr)
                && cheri_address_get(addr) + sizeof(void *) <=
                    cheri_base_get(printed[i]) + cheri_length_get(printed[i])))
            return 1;

    if (i >= sizeof(printed) / sizeof(printed[0])) {
        fprintf(stderr, "Too many capabilities to print");
        exit(1);
    }

    printed[nprinted++] = addr;
    return 0;
}


static void print_capability_tree(void *vaddr, char *name, int depth)
{
    void **ptr, *p;
    uintptr_t end;
    
    if (!cheri_is_valid(vaddr)) return;

    print_address(vaddr, name, depth);

    if (get_pointer_range(vaddr, &ptr, &end))
        for (; (uintptr_t)ptr < end; ptr++)
            if (cheritree_dereference_address(&ptr, &p))
                if (cheri_is_valid(p) && !is_printed(p))
                    print_capability_tree(p, name, depth+1);
}


void cheritree_find_capabilities()
{
    void *regs[32], *ptr;
    char reg[20];
    int i;

    saveregs(regs);
    nprinted = 0;

    for (i = 1; i < sizeof(regs) / sizeof(regs[0]); i++) {
        sprintf(reg, "c%d", i);
        print_capability_tree(regs[i], reg, 0);
    }

    print_capability_tree(regs[0], "c0", 0);
    print_capability_tree(cheri_ddc_get(), "ddc", 0);
    print_capability_tree(cheri_pcc_get(), "pcc", 0);
}
