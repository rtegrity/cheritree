/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_SYMBOL_H_
#define _CHERITREE_SYMBOL_H_

#include <stdint.h>
#include "mapping.h"

struct symbol {
    uintptr_t value;
    int namestr;
    char type;
};

void load_symbols(struct vec *symbols, const char *path);
void print_symbols(struct vec *symbols);
struct symbol *find_symbol(struct mapping *mapping, uintptr_t addr);
void print_symbol(struct symbol *symbol);

#endif /* _CHERITREE_SYMBOL_H_ */
