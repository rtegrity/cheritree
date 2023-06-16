/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

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
#include "module.h"
#include "symbol.h"


static int nmodules;
static struct module modules[256];


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
        if (*path && !strcasecmp(modules[i].path, path)) {
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
