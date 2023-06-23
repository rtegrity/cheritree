/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_H_
#define _CHERITREE_H_

#include <stdlib.h>

extern int cheritree_print_mappings();
extern int cheritree_find_capabilities();
extern void _cheritree_init(void *function, void *stack);

static void cheritree_init() {
    char *cp;
    _cheritree_init(&cheritree_init, &cp);
}

#endif /* _CHERITREE_H_ */
