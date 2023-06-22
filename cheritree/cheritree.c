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


void add_mapping_name(void *function, void *stack, void *heap)
{
    struct mapping *functionmap = find_mapping((uintptr_t)function);
    struct mapping *stackmap = find_mapping((uintptr_t)stack);
    struct mapping *heapmap = find_mapping((uintptr_t)heap);

    if (!functionmap || !*getname(functionmap)) return;

    if (stackmap && !*getname(stackmap)) {
        char buf[1024];
        sprintf(buf, "[%s!stack]", getname(functionmap));
        setname(stackmap, buf);
     }

    if (heapmap && !*getname(heapmap)) {
        char buf[1024];
        sprintf(buf, "[%s!heap]", getname(functionmap));
        setname(heapmap, buf);
    }

    // HACK TODO - should try and do this even if init isn't called
    if (function != &add_mapping_name) {
        static int data;
        char *cp = malloc(1);
        add_mapping_name(&add_mapping_name, &cp, cp);
        free(cp);
    }
}


void print_address(uintptr_t addr)
{
    struct mapping *mapping = resolve_mapping(addr);
    struct symbol *symbol;
    size_t offset;

    if (!mapping || !*getname(mapping)) {
    //    print_mapping(mapping);
        return;
    }

    symbol = find_symbol(mapping, addr);
    offset = addr - (size_t)getbase(mapping);

    if (!*getpath(mapping) || !symbol || !*getname(symbol)) {
        printf("%s+%#zx", getname(mapping), offset);
        return;
    }

    offset -= (size_t)symbol->value;

    if (offset)
        printf("%s!%s+%#zx", getname(mapping), getname(symbol), offset);

    else printf("%s!%s", getname(mapping), getname(symbol));
}


int check_printed(void *addr)
{
    int i = 0;

    for (; i < nprinted; i++)
        if (printed[i] == addr ||
                (cheri_base_get(printed[i]) <= cheri_address_get(addr)
                && cheri_address_get(addr) + sizeof(void *) <= cheri_base_get(printed[i]) + cheri_length_get(printed[i])))
            return 1;

    if (i >= sizeof(printed) / sizeof(printed[0])) {
        fprintf(stderr, "Too many capabilities to print");
        exit(1);
    }

    printed[nprinted++] = addr;
    return 0;
}


void print_capability_tree(void *vaddr, char *prefix)
{
    if (check_printed(vaddr) || !cheri_is_valid(vaddr)) return;

    uintptr_t addr = cheri_address_get(vaddr);
    printf("%s %#p    ", prefix, vaddr);
    print_address(addr);
    printf("\n");

    if (!cheri_is_sentry(vaddr)) {
        void **ptr = (void **)((char *)vaddr - ((size_t)vaddr - cheri_align_up(cheri_base_get(vaddr), sizeof(void *))));
        uintptr_t end = cheri_align_down(cheri_base_get(ptr) + cheri_length_get(ptr), sizeof(void *));

        if (!cheri_is_valid(ptr)) return;

        for (; (uintptr_t)ptr < end; ptr++) {
             if (check_address_valid(&ptr) && cheri_is_valid(*ptr)) {
                char pfx[200];
                sprintf(pfx, "  %s", prefix);
                print_capability_tree(*ptr, pfx);
            }           
        }
    }
}


void find_memory_references()
{
    void *regs[32], *ptr;
    int i;

    saveregs(regs);

    nprinted = 0;

    ptr = cheri_pcc_get();

    if (cheri_is_valid(ptr))
        print_capability_tree(ptr, "pcc");

    for (i = 1; i < sizeof(regs) / sizeof(regs[0]); i++)
        if (cheri_is_valid(regs[i])) {
            char prefix[20];
            sprintf(prefix, "c%d", i);
            print_capability_tree(regs[i], prefix);
        }

    if (cheri_is_valid(regs[0]))
        print_capability_tree(regs[0], "c0");

    ptr = cheri_ddc_get();

    if (cheri_is_valid(ptr))
        print_capability_tree(ptr, "ddc");
}
