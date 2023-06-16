/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_MODULE_H_
#define _CHERITREE_MODULE_H_

#include <stdint.h>

struct module {
    uintptr_t base;
    char *path;
    char *name;
    struct symbol *symbols;
    int nsymbols;
    int maxsymbols;
};

extern void print_module(struct module *module);
extern struct module *add_module(char *path, uintptr_t addr);

#endif /* _CHERITREE_MODULE_H_ */
