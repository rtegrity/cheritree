/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif
#include "mapping.h"
#include "symbol.h"
#include "util.h"


static struct vec mappings;

static int str_to_prot(const char *str);
static char *prot_to_str(char *str, int prot);
static int str_to_flags(const char *str);
static char *flags_to_str(char *str, int type);
static int str_to_type(const char *str);
static char *type_to_str(char *str, int type);


static void print_mapping(struct mapping *mapping)
{
    char prot[CT_PROT_MAXLEN], flags[CT_FLAGS_MAXLEN], type[CT_TYPE_MAXLEN];

    printf("%#" PRIxPTR "-%#" PRIxPTR " %s %s %s %s [base %#" PRIxPTR "]\n",
        mapping->start, mapping->end, prot_to_str(prot, mapping->prot),
        flags_to_str(flags, mapping->flags), type_to_str(type, mapping->type),
        (*getpath(mapping) ? getpath(mapping) : getname(mapping)),
        mapping[mapping->base].start);
}


static void update_mappings(struct vec *v)
{
    for (int i = 0; i < getcount(v); i++) {
        struct mapping *mapping = getmapping(v, i);

        for (int j = 0; j < getcount(&mappings); j++) {
            struct mapping *mp = getmapping(&mappings, j);

            if (mp->start >= mapping->end) break;

            if (mp->start == mapping->start && mp->end == mapping->end &&
                !strcmp(getpath(mp), getpath(mapping))) {
                mapping->namestr = mp->namestr;
                break;
            }
        }
    }

    vec_delete(&mappings);
    mappings = *v;
}


static int add_mapping(struct vec *v, uintptr_t start, uintptr_t end,
    int prot, int flags, int type, char *path)
{
    struct mapping *mapping = (struct mapping *)vec_alloc(v, 1);
    char *cp;

    mapping->start = start;
    mapping->end = end;
    mapping->prot = prot;
    mapping->flags = flags;
    mapping->type = type;

    if (!*path) return 1;

    cp = strrchr(path, '/');
    setpath(mapping, path);
    setname(mapping, cp ? cp+1 : path);

    for (int i = 0; i < getcount(v); i++) {
        struct mapping *mp = getmapping(v, i);
        if (!strcmp(getpath(mp), path)) {
            mapping->base = mp - mapping;
            return 1;
        }
    }

    load_symbols(path);
    return 1;
}


static int load_mapping_procstat(char *buffer, struct vec *v)
{
    char prot[6], flags[6], type[3], path[PATH_MAX];
    uintptr_t start, end;
    char *cp;

    strcpy(path, "");
    if (sscanf(buffer, "%*d %" PRIxPTR " %" PRIxPTR
            " %5s %*d %*d %*d %*d %5s %2s %s", &start, &end, prot, flags,
            type, path) < 5)
        return 1;

    return add_mapping(v, start, end, str_to_prot(prot),
        str_to_flags(flags), str_to_type(type), path);
}


void load_mappings()
{
    char cmd[2048];
    struct vec v;

    sprintf(cmd, "procstat -v %d", getpid());
    vec_init(&v, sizeof(struct mapping), 1000);

    if (!load_array_from_cmd(cmd, load_mapping_procstat, &v)) {
        fprintf(stderr, "Unable to load mappings");
        exit(1);        
    }

    update_mappings(&v);
}


struct mapping *find_mapping(uintptr_t addr)
{
    for (int i = 0; i < getcount(&mappings); i++) {
        struct mapping *mp = getmapping(&mappings, i);
        if (addr >= mp->end) continue;
        if (addr < mp->start) break;
        return mp;
    }

    load_mappings();

    for (int i = 0; i < getcount(&mappings); i++) {
        struct mapping *mp = getmapping(&mappings, i);
        if (addr >= mp->end) continue;
        if (addr < mp->start) break;
        return mp;
    }

    return NULL;
}


void print_mappings()
{
    if (!mappings.addr) load_mappings();

    for (int i = 0; i < getcount(&mappings); i++) {
        struct mapping *mp = getmapping(&mappings, i);

        if (mp->prot != CT_PROT_NONE)
            print_mapping(mp);
    }
}


int check_address_valid(void ***pptr)
{
    uintptr_t addr = (uintptr_t)*pptr;

    for (int i = 0; i < getcount(&mappings); i++) {
        struct mapping *mp = getmapping(&mappings, i);

        if (addr >= mp->end) continue;
        if (addr < mp->start) break;

        if (mp->prot == CT_PROT_NONE) {
            *(char **)pptr += (mp->end - sizeof(void *)) - (size_t)*pptr;
            return 0;
        }

        return 1;
    }

    return 0;
}


uintptr_t mapping_getbase(struct mapping *mapping)
{
    return mapping[mapping->base].start;
}


/*
 *  Convert access protection.
 */

static int str_to_prot(const char *s)
{
    return (s[0] == 'r' ? CT_PROT_READ : 0)
         | (s[1] == 'w' ? CT_PROT_WRITE : 0)
         | (s[2] == 'x' ? CT_PROT_EXEC : 0)
         | (s[3] == 'R' ? CT_PROT_READ_CAP : 0)
         | (s[4] == 'W' ? CT_PROT_WRITE_CAP : 0);
}


static char *prot_to_str(char *s, int prot)
{
    s[0] = (prot & CT_PROT_READ) ? 'r' : '-';
    s[1] = (prot & CT_PROT_WRITE) ? 'w' : '-';
    s[2] = (prot & CT_PROT_EXEC) ? 'x' : '-';
    s[3] = (prot & CT_PROT_READ_CAP) ? 'R' : '-';
    s[4] = (prot & CT_PROT_WRITE_CAP) ? 'W' : '-';
    s[5] = 0;
    return s;
}

/*
 *  Convert mapping flags.
 */

static int str_to_flags(const char *s)
{
    return (s[0] == 'C' ? CT_FLAG_COW : 0)
         | (s[0] == 'G' ? CT_FLAG_GUARD : 0)
         | (s[0] == 'U' ? CT_FLAG_UNMAPPED : 0)
         | (s[1] == 'N' ? CT_FLAG_NEEDS_COPY : 0)
         | (s[2] == 'S' ? CT_FLAG_SUPER : 0)
         | (s[3] == 'U' ? CT_FLAG_GROWS_UP : 0)
         | (s[3] == 'D' ? CT_FLAG_GROWS_DOWN : 0)
         | (s[4] == 'W' ? CT_FLAG_USER_WIRED : 0);
}


static char *flags_to_str(char *s, int flags)
{
    s[0] = (flags & CT_FLAG_COW) ? 'C' :
           (flags & CT_FLAG_GUARD) ? 'G' :
           (flags & CT_FLAG_UNMAPPED) ? 'U' : '-';
    s[1] = (flags & CT_FLAG_NEEDS_COPY) ? 'N' : '-';
    s[2] = (flags & CT_FLAG_SUPER) ? 'S' : '-';
    s[3] = (flags & CT_FLAG_GROWS_UP) ? 'U' :
           (flags & CT_FLAG_GROWS_DOWN) ? 'D' : '-';
    s[4] = (flags & CT_FLAG_USER_WIRED) ? 'W' : '-';
    s[5] = 0;
    return s;
}


/*
 *  Convert mapping type.
 */

static int str_to_type(const char *s)
{
    return !strcmp(s, "--") ? CT_TYPE_NONE :
           !strcmp(s, "df") ? CT_TYPE_DEFAULT :
           !strcmp(s, "vn") ? CT_TYPE_VNODE :
           !strcmp(s, "sw") ? CT_TYPE_SWAP :
           !strcmp(s, "dv") ? CT_TYPE_DEVICE :
           !strcmp(s, "ph") ? CT_TYPE_PHYS :
           !strcmp(s, "dd") ? CT_TYPE_DEAD :
           !strcmp(s, "sg") ? CT_TYPE_SG :
           !strcmp(s, "md") ? CT_TYPE_MGTDEVICE :
           !strcmp(s, "gd") ? CT_TYPE_GUARD : CT_TYPE_UNKNOWN;
}


static char *type_to_str(char *s, int type)
{
    return strcpy(s, (type == CT_TYPE_NONE) ? "--" :
        (type == CT_TYPE_DEFAULT) ? "df" :
        (type == CT_TYPE_VNODE) ? "vn" :
        (type == CT_TYPE_SWAP) ? "sw" :
        (type == CT_TYPE_DEVICE) ? "dv" :
        (type == CT_TYPE_PHYS) ? "ph" :
        (type == CT_TYPE_DEAD) ? "dd" :
        (type == CT_TYPE_SG) ? "sg" :
        (type == CT_TYPE_MGTDEVICE) ? "md" :
        (type == CT_TYPE_GUARD) ? "gd" : "??");
}
