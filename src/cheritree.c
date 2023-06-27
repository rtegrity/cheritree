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


static void print_address(void *vaddr, char *name, void **origin, int depth)
{
    addr_t addr = (addr_t)cheri_address_get(vaddr);
    mapping_t *mapping = cheritree_resolve_mapping(addr);
    symbol_t *symbol;
    addr_t offset;
    int i;

    for (i = 0; i < depth; i++) putc(' ', stdout);

    if (depth) printf("%p: ", origin);
    else printf("%s ", name);

    if (!mapping || !*getname(mapping)) {
        printf("%#p\n", vaddr);
        return;
    }

    symbol = cheritree_find_symbol(getpath(mapping), getbase(mapping), addr);
    offset = addr - (addr_t)getbase(mapping);

    printf("%#p  ", vaddr);

    if (!*getpath(mapping) || !symbol || !*getname(symbol)) {
        if (*getname(mapping) == '[')
            printf("%s+%#" PRIxADDR "\n", getname(mapping), offset);

        else printf("%s!%#" PRIxADDR "\n", getname(mapping), offset);
        return;
    }

    offset -= symbol->value;

    if (offset)
        printf("%s!%s+%#" PRIxADDR "\n", getname(mapping), getname(symbol), offset);

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


static int is_exclude(map_t *exclude, void ***pptr)
{
    addr_t addr = (addr_t)*pptr;
    range_t range;

    if (!cheritree_map_find(exclude, addr, &range))
        return 0;

    *(char **)pptr += (range.end - sizeof(void *)) - addr;
    return 1;
}


static void print_capability_tree(map_t *map, map_t *exclude,
    void *vaddr, char *name, void **origin, int depth)
{
    void **ptr, *p;
    uintptr_t end;
    
    if (!cheri_is_valid(vaddr)) return;

    print_address(vaddr, name, origin, depth);

    if (!depth && is_printed(map, vaddr)) return;

    if (get_pointer_range(vaddr, &ptr, &end))
        for (; (uintptr_t)ptr < end; ptr++)
            if (cheritree_dereference_address(&ptr, &p) && !is_exclude(exclude, &ptr))
                if (cheri_is_valid(p) && !is_printed(map, p))
                    print_capability_tree(map, exclude, p, name, ptr, depth+1);
}


void _cheritree_print_capabilities(void **regs, int nregs)
{
    map_t map, exclude;
    mapping_t *stack;
    char reg[20];
    int i;

    if (nregs > 30)
        _cheritree_init(regs[30], regs);

    cheritree_map_init(&map, 1024);
    cheritree_map_init(&exclude, 100);

    // Exclude cheritree stack frames

    stack = cheritree_resolve_mapping((addr_t)regs);
    cheritree_map_add(&exclude, (stack) ? stack->start : (addr_t)regs,
        (addr_t)(regs + nregs));

    print_capability_tree(&map, &exclude, regs, "csp", NULL, 0);

    for (i = 0; i < nregs && i < 31; i++) {
        sprintf(reg, "c%d", i);
        print_capability_tree(&map, &exclude, regs[i], reg, NULL, 0);
    }

    if (nregs > 31)
        print_capability_tree(&map, &exclude, regs[31], "ddc", NULL, 0);

    cheritree_map_delete(&map);
    cheritree_map_delete(&exclude);
}
