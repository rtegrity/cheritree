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


extern int saveregs(void **);

struct symbol {
    uintptr_t value;
    char *name;
    char type;
};

static struct module {
    uintptr_t base;
    char *path;
    char *name;
    struct symbol *symbols;
    int nsymbols;
    int maxsymbols;
} modules[256];

static int nmodules;

static struct mapping {
    uintptr_t start;
    uintptr_t end;
    char prot[6];
    char flags[6];
    char type[3];
    struct module *module;
} mappings[4096];

static int nmappings;

static void *printed[8192];
static int nprinted;


void print_symbol(struct symbol *symbol)
{
    printf("%#" PRIxPTR " %c %s\n", symbol->value,
        symbol->type, symbol->name);
}


void alloc_symbols(struct module *module, char *cmd)
{
    char buffer[1024];
    FILE *fp;

    if (!*module->path) return;

    sprintf(buffer, "%s | wc -l", cmd);
    fp = popen(buffer, "r");

    if (fp == NULL) {
        fprintf(stderr, "Unable to count symbols\n");
        exit(1);
    }

    fscanf(fp, "%d", &module->maxsymbols);
    fclose(fp);

    module->symbols = calloc(module->maxsymbols, sizeof(struct symbol));

    if (!module->symbols) {
        fprintf(stderr, "Unable to allocate symbols");
        exit(1);
    }
}


void load_symbols(struct module *module)
{
    char buffer[1024];
    FILE *fp;

    if (!*module->path) return;

    sprintf(buffer, "nm -ne %s", module->path);
    alloc_symbols(module, buffer);

    fp = popen(buffer, "r");

    if (fp == NULL) {
        fprintf(stderr, "Unable to run nm\n");
        exit(1);
    }

    while (fgets(buffer, sizeof(buffer), fp)) {
        char type[2], name[1024];
        uintptr_t value;

        if (sscanf(buffer, "%" PRIxPTR " %1s %1023s", &value, type, name) != 3)
            continue;

        if (module->nsymbols >= module->maxsymbols)
            break;

        module->symbols[module->nsymbols].value = value;
        module->symbols[module->nsymbols].type = type[0];
        module->symbols[module->nsymbols].name = strdup(name);
        module->nsymbols++;
    }

    fclose(fp);
}


struct symbol *
find_symbol(struct module *module, uintptr_t addr)
{
    static struct symbol base_symbol = { 0, "base", ' ' };
    int i = 0;

    for (; i < module->nsymbols; i++)
        if (module->base + (size_t)module->symbols[i].value > addr) break;

    return (i) ? &module->symbols[i-1] : &base_symbol;
}


void print_symbols(struct module *module)
{
    for (int i = 0; i < module->nsymbols; i++)
        printf("%#" PRIxPTR " %c %s\n", module->symbols[i].value,
            module->symbols[i].type, module->symbols[i].name);
}


void print_module(struct module *module)
{
    printf("%#" PRIxPTR " %s %s\n",
        module->base, module->name, module->path);
}


struct module *
add_module(char *path, uintptr_t addr)
{
    int i = 0;
    char *cp;

    for (; i < nmodules; i++)
        if (!strcasecmp(modules[i].path, path)) {
            // TODO: adjust base ???
            return &modules[i];
        }

    if (i >= sizeof(modules) / sizeof(modules[0])) {
        fprintf(stderr, "Too many modules");
        exit(1);
    }

    modules[i].path = strdup(path);
    modules[i].base = addr;

    cp = strrchr(path, '/');
    modules[i].name = strdup(cp ? cp+1 : path);
    nmodules++;

    load_symbols(&modules[i]);
    return &modules[i];
}


void print_mapping(struct mapping *mapping)
{
    printf("%#" PRIxPTR "-%#" PRIxPTR " %s %s %s %s\n",
        mapping->start, mapping->end, mapping->prot,
        mapping->flags, mapping->type, mapping->module->path);
}


void add_mapping(uintptr_t start, uintptr_t end,
    char *prot, char *flags, char *type, char *path)
{
    int i = 0;

    for (; i < nmappings; i++) {
        if (start >= mappings[i].end) continue;

        if (start < mappings[i].start) {
            for (int j = nmappings++; j > i; j--)
                mappings[j] = mappings[j-1];
        }

        break;
    }

    if (i >= sizeof(mappings) / sizeof(mappings[0])) {
        fprintf(stderr, "Too many mappings");
        exit(1);
    }

    if (i == nmappings) nmappings++;

    mappings[i].start = start;
    mappings[i].end = end;
    strcpy(mappings[i].prot, prot);
    strcpy(mappings[i].flags, flags);
    strcpy(mappings[i].type, type);
    mappings[i].module = add_module(path, start);
}


void load_mappings()
{
    char buffer[1024];
    FILE *fp;

    sprintf(buffer, "procstat -v %d", getpid());
    
    fp = popen(buffer, "r");

    if (fp == NULL) {
        fprintf(stderr, "Unable to run procstat\n");
        exit(1);
    }

    while (fgets(buffer, sizeof(buffer), fp)) {
        char prot[6], flags[6], type[3], path[PATH_MAX];
        uintptr_t start, end;

        strcpy(path, "");
        int rc = sscanf(buffer, "%*d %" PRIxPTR " %" PRIxPTR
            " %5s %*d %*d %*d %*d %5s %2s %s", &start, &end, prot, flags, type, path);

        if (rc >= 5)
            add_mapping(start, end, prot, flags, type, path);
    }

    fclose(fp);
}


struct mapping *find_mapping(uintptr_t addr)
{
    static struct module unknown_module = { 0, "Unknown", "Unknown" };
    static struct mapping unknown_mapping = {
        0, 0, "-----", "", "", &unknown_module
    };

    for (int i = 0; i < nmappings; i++) {
        if (addr >= mappings[i].end) continue;
        if (addr < mappings[i].start) break;
        return &mappings[i];
    }

    load_mappings();

    for (int i = 0; i < nmappings; i++) {
        if (addr >= mappings[i].end) continue;
        if (addr < mappings[i].start) break;
        return &mappings[i];
    }

    return &unknown_mapping;
}


void print_mappings()
{
    int i;

    load_mappings();

    for (i = 0; i < nmappings; i++)
        print_mapping(&mappings[i]);
}


void print_address(uintptr_t addr)
{
    struct mapping *mapping = find_mapping(addr);
    struct module *module = mapping->module;
    struct symbol *symbol = find_symbol(module, addr);

    if (!*module->name) {
    //    print_mapping(mapping);
        return;
    }

    size_t offset = addr - ((size_t)module->base + (size_t)symbol->value);

    if (offset)
        printf("%s!%s+%#zx", module->name, symbol->name, offset);

    else printf("%s!%s", module->name, symbol->name);
}


int check_printed(void *addr)
{
    int i = 0;

    for (; i < nprinted; i++)
        if (printed[i] == addr ||
                (cheri_base_get(printed[i]) <= cheri_address_get(addr)
                && cheri_address_get(addr) + sizeof(void *) < cheri_base_get(printed[i]) + cheri_length_get(printed[i])))
            return 1;

    if (i >= sizeof(printed) / sizeof(printed[0])) {
        fprintf(stderr, "Too many capabilities to print");
        exit(1);
    }

    printed[nprinted++] = addr;
    return 0;
}


int check_address_valid(void ***pptr)
{
    uintptr_t addr = (uintptr_t)*pptr;

    for (int i = 0; i < nmappings; i++) {
        if (addr >= mappings[i].end) continue;
        if (addr < mappings[i].start) break;

        if (strcmp(mappings[i].prot, "-----") == 0) {
            *(char **)pptr += (mappings[i].end - sizeof(void *)) - (size_t)*pptr;
            return 0;
        }

        return 1;
    }

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

    saveregs(regs);

    nprinted = 0;

    ptr = cheri_pcc_get();

    if (cheri_is_valid(ptr))
        print_capability_tree(ptr, "pcc");

    for (int i = 1; i < sizeof(regs) / sizeof(regs[0]); i++)
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

//  print_mappings();
}
