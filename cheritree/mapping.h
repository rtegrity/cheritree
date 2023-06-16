/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_MAPPING_H_
#define _CHERITREE_MAPPING_H_

#include <stdint.h>

struct mapping {
    uintptr_t start;
    uintptr_t end;
    char prot[6];
    char flags[6];
    char type[3];
    int guard;
    struct module *module;
};

extern void print_mapping(struct mapping *mapping);
extern void load_mappings();
extern struct mapping *find_mapping(uintptr_t addr);
extern void print_mappings();
extern int check_address_valid(void ***pptr);

#endif /* _CHERITREE_MAPPING_H_ */
