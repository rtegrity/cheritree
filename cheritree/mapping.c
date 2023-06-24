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
#include "mapping.h"
#include "symbol.h"
#include "util.h"


static struct vec mappings;

static void load_mappings();
static void flags_to_str(int flags, char *s, size_t len);
static int str_to_flags(char *s, size_t len);


static mapping_t *find_mapping(addr_t addr)
{
    int i;

    if (!mappings.addr) load_mappings();

    for (i = 0; i < getcount(&mappings); i++) {
        mapping_t *mp = getmapping(&mappings, i);
        if (addr >= mp->end) continue;
        if (addr < mp->start) break;
        return mp;
    }

    return NULL;
}


void cheritree_set_mapping_name(mapping_t *mapping,
    mapping_t *owner, const char *name)
{
    char buf[2048];

    if (!name || !*name) return;
    if (!mapping || *getpath(mapping)) return;

    if (strchr(getname(mapping), '!')) return;

    if (owner && *getname(owner))
        sprintf(buf, "[%s!%s]", getname(owner), name);
    
    else sprintf(buf, "[%s]", name);
    setname(mapping, buf);
}


static void add_mapping_name(mapping_t *mapping)
{
    addr_t start = mapping->start, end = mapping->end;

    // Copy any previously identified name

    if (getcount(&mappings)) {
        mapping_t *mp = find_mapping(start);

        if (mp && mp->start == start && mp->end == end && !*getpath(mp)) {
            mapping->namestr = mp->namestr;
            return;
        }
    }

    // Check for current stack mapping

    if (start <= (addr_t)&mapping && (addr_t)&mapping < end) {
        cheritree_set_mapping_name(mapping, NULL, "stack");
        return;
    }

    // Check for current heap mapping

    if (start <= (addr_t)mapping && (addr_t)mapping < end)
        cheritree_set_mapping_name(mapping, NULL, "heap");
}


static int add_mapping(struct vec *v, addr_t start,
    addr_t end, int flags, char *path)
{
    mapping_t *mapping = (mapping_t *)cheritree_vec_alloc(v, 1);
    mapping_t *base = NULL;
    char *cp;
    int i;

    mapping->start = start;
    mapping->end = end;
    mapping->flags = flags;

    // Identify base mapping

    for (i = 0; i < getcount(v); i++) {
        mapping_t *mp = getmapping(v, i);

        if (!mp->base && *getpath(mp)) base = mp;

        if (*path && !strcmp(getpath(mp), path)) {
            mapping->base = mp - mapping;
            break;
        }
    }

    // Mappings included in base symbols

    if (!*path && getprot(mapping) != CT_PROT_NONE) {
        if (base && cheritree_find_type(getpath(base),
                getbase(base), start, end) != NULL) {
            mapping->base = base - mapping;
            mapping->namestr = base->namestr;
            return 1;
        }
    }

    if (!*path) {
        add_mapping_name(mapping);
        return 1;
    }

    if (*path == '[') {
        setname(mapping, path);
        return 1;
    }

    cp = strrchr(path, '/');
    setpath(mapping, path);
    setname(mapping, cp ? cp+1 : path);

    cheritree_load_symbols(path);
    return 1;
}


#ifdef __FreeBSD__
static struct flagmap { int i; char s[6]; int f; } flagmap[] = {
    { 0, "-----", 0 }, { 0, "r", CT_PROT_READ },
    { 1, "w", CT_PROT_WRITE }, { 2, "x", CT_PROT_EXEC },
    { 3, "R", CT_PROT_READ_CAP }, { 4, "W", CT_PROT_WRITE_CAP },

    { 6, "-----", 0 }, { 6, "U", CT_FLAG_UNMAPPED },
    { 6, "G", CT_FLAG_GUARD }, { 6, "C", CT_FLAG_COW },
    { 7, "N", CT_FLAG_NEEDS_COPY }, { 8, "S", CT_FLAG_SUPER },
    { 9, "D", CT_FLAG_GROWS_DOWN }, { 9, "U", CT_FLAG_GROWS_UP },
    { 10, "W", CT_FLAG_USER_WIRED },

    { 12, "--", 0 }, { 12, "df", CT_TYPE_DEFAULT },
    { 12, "vn", CT_TYPE_VNODE }, { 12, "sw", CT_TYPE_SWAP },
    { 12, "dv", CT_TYPE_DEVICE }, { 12, "ph", CT_TYPE_PHYS },
    { 12, "dd", CT_TYPE_DEAD }, { 12, "sg", CT_TYPE_SG },
    { 12, "md", CT_TYPE_MGTDEVICE }, { 12, "gd", CT_TYPE_GUARD },
    { 12, "??", CT_TYPE_UNKNOWN },
    { 0 }
};


static void print_mapping(mapping_t *mapping)
{
    char s[15];

    flags_to_str(getflags(mapping), s, sizeof(s));

    printf("%#" PRIxADDR "-%#" PRIxADDR " %s %s %s %s [base %#" PRIxADDR "]\n",
        mapping->start, mapping->end, &s[0], &s[6], &s[12],
        (*getpath(mapping) ? getpath(mapping) : getname(mapping)),
        mapping[mapping->base].start);
}


static int load_mapping(char *buffer, struct vec *v)
{
    char s[15], path[PATH_MAX];
    addr_t start, end;

    strcpy(path, "");
    memset(s, 0, sizeof(s));

    if (sscanf(buffer, "%*d %" PRIxADDR " %" PRIxADDR
            " %5s %*d %*d %*d %*d %5s %2s %s", &start, &end,
            &s[0], &s[6], &s[12], path) < 5)
        return 1;

    return add_mapping(v, start, end, str_to_flags(s, sizeof(s)), path);
}


static void load_mappings()
{
    char cmd[2048];
    struct vec v;

    sprintf(cmd, "procstat -v %d", getpid());
    cheritree_vec_init(&v, sizeof(mapping_t), 1024);

    if (!cheritree_load_from_cmd(cmd, load_mapping, &v)) {
        fprintf(stderr, "Unable to load mappings");
        exit(1);        
    }

    cheritree_vec_delete(&mappings);
    mappings = v;
}
#endif /* __FreeBSD__ */


#ifdef __linux__
static struct flagmap { int i; char s[5]; int f; } flagmap[] = {
    { 0, "----", 0 }, { 0, "r", CT_PROT_READ },
    { 1, "w", CT_PROT_WRITE }, { 2, "x", CT_PROT_EXEC },
    { 3, "p", CT_FLAG_SHARED }, { 3, "p", CT_FLAG_PRIVATE },
    { 0 }
};


static void print_mapping(mapping_t *mapping)
{
    char s[5];

    flags_to_str(getflags(mapping), s, sizeof(s));

    printf("%#" PRIxADDR "-%#" PRIxADDR " %s %s [base %#" PRIxADDR "]\n",
        mapping->start, mapping->end, s,
        (*getpath(mapping) ? getpath(mapping) : getname(mapping)),
        mapping[mapping->base].start);
}


static int load_mapping(char *buffer, struct vec *v)
{
    char s[5], path[PATH_MAX];
    addr_t start, end;

    strcpy(path, "");
    memset(s, 0, sizeof(s));

    if (sscanf(buffer, "%" PRIxADDR "-%" PRIxADDR
            " %4s %*x %*d:%*d %*d %s", &start, &end, s, path) < 4)
        return 1;

    return add_mapping(v, start, end, str_to_flags(s, sizeof(s)), path);
}


static void load_mappings()
{
    char path[2048];
    struct vec v;

    sprintf(path, "/proc/%d/maps", getpid());
    cheritree_vec_init(&v, sizeof(mapping_t), 1024);

    if (!cheritree_load_from_path(path, load_mapping, &v)) {
        fprintf(stderr, "Unable to load mappings");
        exit(1);        
    }

    cheritree_vec_delete(&mappings);
    mappings = v;
}
#endif /* __linux__ */


mapping_t *cheritree_resolve_mapping(addr_t addr)
{
    mapping_t *mapping = find_mapping(addr);

    if (!mapping) {
        load_mappings();
        return find_mapping(addr);
    }

    return mapping;
}


void cheritree_print_mappings()
{
    int i;

    if (!mappings.addr) load_mappings();

    for (i = 0; i < getcount(&mappings); i++) {
        mapping_t *mp = getmapping(&mappings, i);

        if (getprot(mp) != CT_PROT_NONE)
            print_mapping(mp);
    }
}


int cheritree_dereference_address(void ***pptr, void **paddr)
{
    addr_t addr = (addr_t)*pptr;
    mapping_t *mapping = cheritree_resolve_mapping(addr);

    if (!mapping) return 0;

    if (getprot(mapping) == CT_PROT_NONE) {
        *(char **)pptr += (mapping->end - sizeof(void *)) - (addr_t)*pptr;
         return 0;
    }

    *paddr = **pptr;
    return 1;
}


static void flags_to_str(int flags, char *s, size_t len)
{
    int type = (flags & CT_TYPE_MASK);
    struct flagmap *fp;

    memset(s, 0, len);

    for (fp = flagmap; *fp->s; fp++)
        if ((fp->f & CT_TYPE_MASK) ? (type == fp->f) : (flags & fp->f) == fp->f)
            if (fp->i + strlen(fp->s) < len)
                strncpy(&s[fp->i], fp->s, strlen(fp->s));
}


static int str_to_flags(char *s, size_t len)
{
    struct flagmap *fp;
    int flags = 0;

    for (fp = flagmap; *fp->s; fp++)
        if (fp->f && fp->i + strlen(fp->s) < len)
            if (!strncmp(&s[fp->i], fp->s, strlen(fp->s)))
                flags |= fp->f;

    return flags;
}
