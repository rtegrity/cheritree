/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_H_
#define _CHERITREE_H_

#include <stdlib.h>

extern int print_mappings();
extern int find_memory_references();

extern void add_mapping_name(void *function, void *stack, void *heap);

static void cheritree_init() {
    char *cp = malloc(1);
    add_mapping_name(&cheritree_init, &cp, cp);
    free(cp);
}

#endif /* _CHERITREE_H_ */
