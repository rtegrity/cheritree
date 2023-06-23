/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_SYMBOL_H_
#define _CHERITREE_SYMBOL_H_

#include <stdint.h>
#include "util.h"


/*
 *  Symbol store.
 */
struct image {
    struct vec symbols;     // Symbols
    string_t pathstr;       // Pathname
};

struct symbol {
    uintptr_t value;        // Symbol value
    string_t namestr;       // Symbol name
    char type;              // Type
};

void cheritree_load_symbols(const char *path);
void cheritree_print_symbols(const char *path);
struct symbol *cheritree_find_symbol(const char *path, uintptr_t base, uintptr_t addr);
const char *cheritree_find_type(const char *path, uintptr_t base, uintptr_t start, uintptr_t end);


/*
 *  Access functions.
 */
#define getsymbol(v,i)      (struct symbol *)cheritree_vec_get((v),(i))
#define getimage(v,i)       (struct image *)cheritree_vec_get((v),(i))

#endif /* _CHERITREE_SYMBOL_H_ */
