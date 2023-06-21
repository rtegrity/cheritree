/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_MAPPING_H_
#define _CHERITREE_MAPPING_H_

#include <stdint.h>
#include <unistd.h>
#include <sys/user.h>
#include "util.h"


/*
 *  Namespace definitions
 */

#define load_mappings           cheritree_load_mappings
#define find_mapping            find_mapping
#define resolve_mapping         resolve_mapping
#define print_mappings          print_mappings
#define check_address_valid     cheritree_check_address_valid


/*
 *  Memory mapping.
 */
struct mapping {
    uintptr_t start;
    uintptr_t end;
    int prot;
    int flags;
    int type;
    int base;
    string_t pathstr;
    string_t namestr;
};

void load_mappings();
struct mapping *find_mapping(uintptr_t addr);
struct mapping *resolve_mapping(uintptr_t addr);
void print_mappings();
int check_address_valid(void ***pptr);


/*
 *  Access protection.
 */
#define CT_PROT_NONE            0
#define CT_PROT_READ            KVME_PROT_READ
#define CT_PROT_WRITE           KVME_PROT_WRITE
#define CT_PROT_EXEC            KVME_PROT_EXEC
#define CT_PROT_READ_CAP        KVME_PROT_READ_CAP
#define CT_PROT_WRITE_CAP       KVME_PROT_WRITE_CAP

#define CT_PROT_MAXLEN          6

#define kvme_to_prot(x)         (x)


/*
 *  Mapping flags.
 */
#define CT_FLAG_COW             KVME_FLAG_COW
#define CT_FLAG_GUARD           KVME_FLAG_GUARD
#define CT_FLAG_UNMAPPED        KVME_FLAG_UNMAPPED
#define CT_FLAG_NEEDS_COPY      KVME_FLAG_NEEDS_COPY
#define CT_FLAG_SUPER           KVME_FLAG_SUPER
#define CT_FLAG_GROWS_UP        KVME_FLAG_GROWS_UP
#define CT_FLAG_GROWS_DOWN      KVME_FLAG_GROWS_DOWN
#define CT_FLAG_USER_WIRED      KVME_FLAG_USER_WIRED

#define CT_FLAGS_MAXLEN         6

#define kvme_to_flags(x)        (x)


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

#define CT_TYPE_MAXLEN          3

#define kvme_to_type(x)         (x)


/*
 *  Access functions.
 */
#define getmapping(v,i)     (struct mapping *)vec_get((v),(i))
#define getbase(m)          ((m) ? (m)[(m)->base].start : 0)

#endif /* _CHERITREE_MAPPING_H_ */
