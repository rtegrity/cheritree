/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_SYMBOL_H_
#define _CHERITREE_SYMBOL_H_

#include <stdint.h>
#include "module.h"

struct symbol {
    uintptr_t value;
    char *name;
    char type;
};

extern void load_symbols(struct module *module);
extern void print_symbols(struct module *module);
extern struct symbol *find_symbol(struct module *module, uintptr_t addr);
extern void print_symbol(struct symbol *symbol);

#endif /* _CHERITREE_SYMBOL_H_ */
