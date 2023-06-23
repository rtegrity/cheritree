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
 *  Namespace definitions
 */
#define load_symbols        cheritree_load_symbols
#define print_symbols       cheritree_print_symbols
#define find_symbol         cheritree_find_symbol
#define find_type           cheritree_find_type


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

void load_symbols(const char *path);
void print_symbols(const char *path);
struct symbol *find_symbol(const char *path, uintptr_t base, uintptr_t addr);
const char *find_type(const char *path, uintptr_t base, uintptr_t start, uintptr_t end);


/*
 *  Access functions.
 */
#define getsymbol(v,i)      (struct symbol *)vec_get((v),(i))
#define getimage(v,i)       (struct image *)vec_get((v),(i))

#endif /* _CHERITREE_SYMBOL_H_ */
