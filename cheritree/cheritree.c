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

    printf("%s %#p  ", name, vaddr);

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


static int is_printed(map_t *map, void *addr)
{
    addr_t start = cheri_base_get(addr);
    addr_t end = start + cheri_length_get(addr);

    return !cheritree_map_add(map, start, end);
}


static void print_capability_tree(map_t *map,
    void *vaddr, char *name, int depth)
{
    void **ptr, *p;
    uintptr_t end;
    
    if (!cheri_is_valid(vaddr)) return;

    print_address(vaddr, name, depth);

    if (get_pointer_range(vaddr, &ptr, &end))
        for (; (uintptr_t)ptr < end; ptr++)
            if (cheritree_dereference_address(&ptr, &p))
                if (cheri_is_valid(p) && !is_printed(map, p))
                    print_capability_tree(map, p, name, depth+1);
}


void _cheritree_find_capabilities(void **regs, int nregs)
{
    char reg[20];
    map_t map;
    int i;

    if (nregs > 30)
        _cheritree_init(regs[30], regs);

    cheritree_map_init(&map, 1024);
    cheritree_map_add(&map, (addr_t)regs, (addr_t)&regs[nregs]);

    print_capability_tree(&map, regs, "csp", 0);

    for (i = 0; i < nregs && i < 32; i++) {
        sprintf(reg, "c%d", i);
        print_capability_tree(&map, regs[i], reg, 0);
    }

    if (nregs > 32)
        print_capability_tree(&map, regs[32], "ddc", 0);

    cheritree_map_print(&map);
    cheritree_map_delete(&map);
}
