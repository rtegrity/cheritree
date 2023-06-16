/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_MAPPING_H_
#define _CHERITREE_MAPPING_H_

#include <stdint.h>
#include <sys/user.h>

/*
 *  Memory mapping.
 */
struct mapping {
    uintptr_t start;
    uintptr_t end;
    char _prot[6];
    char flags[6];
    char type[3];
    int guard;
    struct module *module;
};

/*
 *  Access protection.
 */
#define CT_PROT_READ            KVME_PROT_READ
#define CT_PROT_WRITE           KVME_PROT_WRITE
#define CT_PROT_EXEC            KVME_PROT_EXEC
#define CT_PROT_READ_CAP        KVME_PROT_READ_CAP
#define CT_PROT_WRITE_CAP       KVME_PROT_WRITE_CAP

#define kvme_to_prot(x)         (x)
#define prot_to_kvme(x)         (x)

/*
 *  Mapping type.
 */
#define CT_TYPE_NONE            KVME_TYPE_NONE
#define CT_TYPE_DEFAULT         KVME_TYPE_DEFAULT
#define CT_TYPE_VNODE           KVME_TYPE_VNODE
#define CT_TYPE_SWAP            KVME_TYPE_SWAP
#define CT_TYPE_DEVICE          KVME_TYPE_DEVICE
#define CT_TYPE_PHYS            KVME_TYPE_PHYS
#define CT_TYPE_DEAD            KVME_TYPE_DEAD
#define CT_TYPE_SG              KVME_TYPE_SG
#define CT_TYPE_MGTDEVICE       KVME_TYPE_MGTDEVICE
#define CT_TYPE_GUARD           KVME_TYPE_GUARD
#define CT_TYPE_UNKNOWN         KVME_TYPE_UNKNOWN

#define kvme_to_type(x)         (x)
#define type_to_kvme(x)         (x)

extern void print_mapping(struct mapping *mapping);
extern void load_mappings();
extern struct mapping *find_mapping(uintptr_t addr);
extern void print_mappings();
extern int check_address_valid(void ***pptr);

#endif /* _CHERITREE_MAPPING_H_ */
