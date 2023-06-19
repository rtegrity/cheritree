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
#include "module.h"


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
    pclose(fp);

    module->symbols = calloc(module->maxsymbols, sizeof(struct symbol));

    if (!module->symbols) {
        fprintf(stderr, "Unable to allocate symbols");
        exit(1);
    }
}


void load_symbols(struct module *module)
{
    char buffer[2048];
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

    pclose(fp);
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
