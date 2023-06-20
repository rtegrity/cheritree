/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif
#include "symbol.h"
#include "mapping.h"
#include "util.h"


void print_symbol(struct symbol *symbol)
{
    printf("%#" PRIxPTR " %c %s\n", symbol->value,
        symbol->type, symbol->name);
}


int load_symbol(char *buffer, struct vec *v)
{
    char type[2], name[1024];
    uintptr_t value;

    if (sscanf(buffer, "%" PRIxPTR " %1s %1023s", &value, type, name) != 3)
        return 1;

    struct symbol *symbol = (struct symbol *)vec_alloc(v, 1);
    if (!symbol) return 0;

    symbol->namestr = string_alloc(name);
    if (!symbol->namestr) return 0;

    symbol->value = value;
    symbol->type = type[0];
    symbol->name = strdup(name);
    return 1;
}


void load_symbols(struct vec *symbols, const char *path)
{
    char cmd[2048];

    if (!*path) return;

    sprintf(cmd, "nm -ne %s", path);
    vec_init(symbols, sizeof(struct symbol), 1000);

    if (!load_array_from_cmd(cmd, load_symbol, symbols)) {
        fprintf(stderr, "Unable to load symbols");
        exit(1);
    }

// HACK
    printf("---- %s ----\n", path);
    print_symbols(symbols);
}


struct symbol *
find_symbol(struct mapping *mapping, uintptr_t addr)
{
    static struct symbol base_symbol = { 0, "base", ' ' };
    struct mapping *base = &mapping[mapping->base];
    struct vec *symbols = &base->symbols;
    int i = 0;

    for (; i < vec_getcount(symbols); i++) {
        struct symbol *sym = (struct symbol *)vec_get(symbols, i);
        if (sym && base->start + (size_t)sym->value > addr) break;
    }

    return (i) ? (struct symbol *)vec_get(symbols, i) : &base_symbol;
}


void print_symbols(struct vec *symbols)
{
    for (int i = 0; i < vec_getcount(symbols); i++) {
        struct symbol *sym = (struct symbol *)vec_get(symbols, i);
        printf("%#" PRIxPTR " %c %s\n", sym->value, sym->type,
            string_get(sym->namestr));
    }
}
