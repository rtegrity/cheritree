/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_MAPPING_H_
#define _CHERITREE_MAPPING_H_

#include <stdint.h>
#include <unistd.h>
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
    uintptr_t start;            // Start address
    uintptr_t end;              // End address
    int flags;                  // Mapping flags
    int base;                   // Start of image (offset)
    string_t pathstr;           // Path string
    string_t namestr;           // Name string
};

void load_mappings();
struct mapping *find_mapping(uintptr_t addr);
struct mapping *resolve_mapping(uintptr_t addr);
void print_mappings();
int check_address_valid(void ***pptr, void **paddr);


/*
 *  Mapping type.
 */
#define CT_TYPE_NONE            0
#define CT_TYPE_DEFAULT         1
#define CT_TYPE_VNODE           2
#define CT_TYPE_SWAP            3
#define CT_TYPE_DEVICE          4
#define CT_TYPE_PHYS            5
#define CT_TYPE_DEAD            6
#define CT_TYPE_SG              7
#define CT_TYPE_MGTDEVICE       8
#define CT_TYPE_GUARD           9
#define CT_TYPE_UNKNOWN         10
#define CT_TYPE_MASK            0xff


/*
 *  Access protection.
 */
#define CT_PROT_NONE            0
#define CT_PROT_EXEC            0x0100
#define CT_PROT_WRITE           0x0200
#define CT_PROT_READ            0x0400
#define CT_PROT_WRITE_CAP       0x0800
#define CT_PROT_READ_CAP        0x1000
#define CT_PROT_MASK            0xff00


/*
 *  Mapping flags.
 */
#define CT_FLAG_COW             0x00010000
#define CT_FLAG_GUARD           0x00020000
#define CT_FLAG_UNMAPPED        0x00040000
#define CT_FLAG_NEEDS_COPY      0x00080000
#define CT_FLAG_SUPER           0x00100000
#define CT_FLAG_GROWS_UP        0x00200000
#define CT_FLAG_GROWS_DOWN      0x00400000
#define CT_FLAG_USER_WIRED      0x00800000
#define CT_FLAG_SHARED          0x01000000
#define CT_FLAG_PRIVATE         0x02000000


/*
 *  Access functions.
 */
#define getmapping(v,i)     (struct mapping *)vec_get((v),(i))
#define getbase(m)          ((m) ? (m)[(m)->base].start : 0)
#define gettype(m)          ((m) ? ((m)->flags & CT_TYPE_MASK) : 0)
#define getprot(m)          ((m) ? ((m)->flags & CT_PROT_MASK) : 0)
#define getflags(m)         ((m) ? (m)->flags : 0)


#endif /* _CHERITREE_MAPPING_H_ */
