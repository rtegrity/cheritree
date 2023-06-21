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


/*
 *  Namespace definitions
 */
#define load_symbols        cheritree_load_symbols
#define print_symbols       cheritree_print_symbols
#define find_symbol         cheritree_find_symbol

/*
 *  Symbol store.
 */
struct image {
    struct vec symbols;
    string_t pathstr;
};

struct symbol {
    uintptr_t value;
    string_t namestr;
    char type;
};

void load_symbols(const char *path);
void print_symbols(const char *path);
struct symbol *find_symbol(const struct mapping *mapping, uintptr_t addr);


/*
 *  Access functions.
 */
#define getsymbol(v,i)      (struct symbol *)vec_get((v),(i))
#define getimage(v,i)       (struct image *)vec_get((v),(i))

#endif /* _CHERITREE_SYMBOL_H_ */
