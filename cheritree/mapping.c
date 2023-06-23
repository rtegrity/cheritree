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

static void flags_to_str(int flags, char *s, size_t len);
static int str_to_flags(char *s, size_t len);


struct mapping *find_mapping(uintptr_t addr)
{
    int i;

    if (!mappings.addr) load_mappings();

    for (i = 0; i < getcount(&mappings); i++) {
        struct mapping *mp = getmapping(&mappings, i);
        if (addr >= mp->end) continue;
        if (addr < mp->start) break;
        return mp;
    }

    return NULL;
}


void set_mapping_name(struct mapping *mapping,
    struct mapping *owner, const char *name)
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


static void add_mapping_name(struct mapping *mapping)
{
    uintptr_t start = mapping->start, end = mapping->end;

    // Copy any previously identified name

    if (getcount(&mappings)) {
        struct mapping *mp = find_mapping(start);

        if (mp && mp->start == start && mp->end == end && !*getpath(mp)) {
            mapping->namestr = mp->namestr;
            return;
        }
    }

    // Check for current stack mapping

    if (start <= (uintptr_t)&mapping && (uintptr_t)&mapping < end) {
        set_mapping_name(mapping, NULL, "stack");
        return;
    }

    // Check for current heap mapping

    if (start <= (uintptr_t)mapping && (uintptr_t)mapping < end)
        set_mapping_name(mapping, NULL, "heap");
}


static int add_mapping(struct vec *v, uintptr_t start,
    uintptr_t end, int flags, char *path)
{
    struct mapping *mapping = (struct mapping *)vec_alloc(v, 1);
    struct mapping *base = NULL;
    char *cp;
    int i;

    mapping->start = start;
    mapping->end = end;
    mapping->flags = flags;

    // Identify base mapping

    for (i = 0; i < getcount(v); i++) {
        struct mapping *mp = getmapping(v, i);

        if (!mp->base && *getpath(mp)) base = mp;

        if (*path && !strcmp(getpath(mp), path)) {
            mapping->base = mp - mapping;
            break;
        }
    }

    // Mappings included in base symbols

    if (!*path && getprot(mapping) != CT_PROT_NONE) {
        if (base && find_type(base, start, end) != NULL) {
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

    load_symbols(path);
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


static void print_mapping(struct mapping *mapping)
{
    char s[15];

    flags_to_str(getflags(mapping), s, sizeof(s));

    printf("%#" PRIxPTR "-%#" PRIxPTR " %s %s %s %s [base %#" PRIxPTR "]\n",
        mapping->start, mapping->end, &s[0], &s[6], &s[12],
        (*getpath(mapping) ? getpath(mapping) : getname(mapping)),
        mapping[mapping->base].start);
}


static int load_mapping(char *buffer, struct vec *v)
{
    char s[15], path[PATH_MAX];
    uintptr_t start, end;

    strcpy(path, "");
    memset(s, 0, sizeof(s));

    if (sscanf(buffer, "%*d %" PRIxPTR " %" PRIxPTR
            " %5s %*d %*d %*d %*d %5s %2s %s", &start, &end,
            &s[0], &s[6], &s[12], path) < 5)
        return 1;

    return add_mapping(v, start, end, str_to_flags(s, sizeof(s)), path);
}


void load_mappings()
{
    char cmd[2048];
    struct vec v;

    sprintf(cmd, "procstat -v %d", getpid());
    vec_init(&v, sizeof(struct mapping), 1024);

    if (!load_array_from_cmd(cmd, load_mapping, &v)) {
        fprintf(stderr, "Unable to load mappings");
        exit(1);        
    }

    vec_delete(&mappings);
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


static void print_mapping(struct mapping *mapping)
{
    char s[5];

    flags_to_str(getflags(mapping), s, sizeof(s));

    printf("%#" PRIxPTR "-%#" PRIxPTR " %s %s [base %#" PRIxPTR "]\n",
        mapping->start, mapping->end, s,
        (*getpath(mapping) ? getpath(mapping) : getname(mapping)),
        mapping[mapping->base].start);
}


static int load_mapping(char *buffer, struct vec *v)
{
    char s[5], path[PATH_MAX];
    uintptr_t start, end;

    strcpy(path, "");
    memset(s, 0, sizeof(s));

    if (sscanf(buffer, "%" PRIxPTR "-%" PRIxPTR
            " %4s %*x %*d:%*d %*d %s", &start, &end, s, path) < 4)
        return 1;

    return add_mapping(v, start, end, str_to_flags(s, sizeof(s)), path);
}


void load_mappings()
{
    char path[2048];
    struct vec v;

    sprintf(path, "/proc/%d/maps", getpid());
    vec_init(&v, sizeof(struct mapping), 1024);

    if (!load_array_from_path(path, load_mapping, &v)) {
        fprintf(stderr, "Unable to load mappings");
        exit(1);        
    }

    vec_delete(&mappings);
    mappings = v;
}
#endif /* __linux__ */


struct mapping *resolve_mapping(uintptr_t addr)
{
    struct mapping *mapping = find_mapping(addr);

    if (!mapping) {
        load_mappings();
        return find_mapping(addr);
    }

    return mapping;
}


void print_mappings()
{
    int i;

    if (!mappings.addr) load_mappings();

    for (i = 0; i < getcount(&mappings); i++) {
        struct mapping *mp = getmapping(&mappings, i);

        if (getprot(mp) != CT_PROT_NONE)
            print_mapping(mp);
    }
}


int check_address_valid(void ***pptr, void **paddr)
{
    uintptr_t addr = (uintptr_t)*pptr;
    struct mapping *mapping = resolve_mapping(addr);

    if (!mapping) return 0;

    if (getprot(mapping) == CT_PROT_NONE) {
        *(char **)pptr += (mapping->end - sizeof(void *)) - (size_t)*pptr;
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
