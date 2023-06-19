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
#include "util.h"


void print_symbol(struct symbol *symbol)
{
    printf("%#" PRIxPTR " %c %s\n", symbol->value,
        symbol->type, symbol->name);
}


int load_symbol(char *buffer, void *element, void *context)
{
    struct symbol *symbol = (struct symbol *)element;
    char type[2], name[1024];
    uintptr_t value;

    if (sscanf(buffer, "%" PRIxPTR " %1s %1023s", &value, type, name) != 3)
        return 0;

    symbol->value = value;
    symbol->type = type[0];
    symbol->name = strdup(name);
    return 1;
}


void load_symbols(struct module *module)
{
    char cmd[2048];

    if (!*module->path) return;

    sprintf(cmd, "nm -ne %s", module->path);

    module->symbols = (struct symbol *)load_array_from_cmd(
        cmd, load_symbol, NULL,
        sizeof(struct symbol), &module->nsymbols, 1000);   /* HACK */

    if (module->symbols == NULL) {
        fprintf(stderr, "Unable to load symbols");
        exit(1);
    }

// HACK
    printf("---- %s ----\n", module->path);
    print_symbols(module);
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
