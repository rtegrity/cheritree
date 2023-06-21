/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_SYMBOL_H_
#define _CHERITREE_SYMBOL_H_

#include <stdint.h>
#include "mapping.h"
#include "util.h"

struct symbol {
    uintptr_t value;
    string_t namestr;
    char type;
};

void load_symbols(const char *path);
void print_symbols(const char *path);
struct symbol *find_symbol(struct mapping *mapping, uintptr_t addr);

#define getsymbol(v,i)      (struct symbol *)vec_get((v),(i))
#define getimage(v,i)       (struct image *)vec_get((v),(i))

#endif /* _CHERITREE_SYMBOL_H_ */
