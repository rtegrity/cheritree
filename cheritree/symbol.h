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
typedef struct image {
    struct vec symbols;     // Symbols
    string_t pathstr;       // Pathname
} image_t;

typedef struct symbol {
    uintptr_t value;        // Symbol value
    string_t namestr;       // Symbol name
    char type;              // Type
} symbol_t;

void cheritree_load_symbols(const char *path);
void cheritree_print_symbols(const char *path);
symbol_t *cheritree_find_symbol(const char *path, uintptr_t base, uintptr_t addr);
const char *cheritree_find_type(const char *path, uintptr_t base, uintptr_t start, uintptr_t end);


/*
 *  Access functions.
 */
#define getsymbol(v,i)      (symbol_t *)cheritree_vec_get((v),(i))
#define getimage(v,i)       (image_t *)cheritree_vec_get((v),(i))

#endif /* _CHERITREE_SYMBOL_H_ */
