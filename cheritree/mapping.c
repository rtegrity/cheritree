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
#include <sys/sysctl.h>
#include <sys/user.h>
#include <libprocstat.h>
#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif
#include "mapping.h"
#include "symbol.h"
#include "util.h"


static int nmappings;
static struct mapping mappings[4096];

static int str_to_prot(const char *str);
static char *prot_to_str(char *str, int prot);
static int str_to_flags(const char *str);
static char *flags_to_str(char *str, int type);
static int str_to_type(const char *str);
static char *type_to_str(char *str, int type);


void print_mapping(struct mapping *mapping)
{
    char prot[CT_PROT_MAXLEN], flags[CT_FLAGS_MAXLEN], type[CT_TYPE_MAXLEN];

    printf("%#" PRIxPTR "-%#" PRIxPTR " %s %s %s %s\n",
        mapping->start, mapping->end, prot_to_str(prot, mapping->prot),
        flags_to_str(flags, mapping->flags), type_to_str(type, mapping->type),
        (*mapping_getpath(mapping) ? mapping_getpath(mapping) :
            mapping_getname(mapping)));
}


void add_mapping(uintptr_t start, uintptr_t end,
    int prot, int flags, int type, char *path)
{
    int i = 0;
    char *cp;

    for (; i < nmappings; i++) {
        if (start >= mappings[i].end) continue;

        if (start < mappings[i].start) {
            for (int j = nmappings++; j > i; j--)
                mappings[j] = mappings[j-1];
        }

        break;
    }

    if (i >= sizeof(mappings) / sizeof(mappings[0])) {
        fprintf(stderr, "Too many mappings");
        exit(1);
    }

    if (i == nmappings) nmappings++;

    if (mappings[i].start == start && mappings[i].end == end)
        return;

    mappings[i].start = start;
    mappings[i].end = end;
    mappings[i].prot = prot;
    mappings[i].flags = flags;
    mappings[i].type = type;

    cp = strrchr(path, '/');
    mappings[i].pathstr = string_alloc(path);
    mappings[i].namestr = string_alloc(cp ? cp+1 : path);

    for (int j = 0; j < nmappings; j++) {
        struct mapping *mp = &mappings[j];
        if (!strcmp(path, string_get(mp->pathstr))) {
            mappings[i].base = i - j;
            break;
        }
    }

    if (!mappings[i].base && *path)
        load_symbols(&mappings[i].symbols, path);
}


#if 0
void load_mappings_procstat()
{
    struct procstat *stat = procstat_open_sysctl();
    unsigned int count;

    if (!stat) {
        fprintf(stderr, "Unable to open procstat\n");
        exit(1);
    }

    struct kinfo_proc *proc = procstat_getprocs(stat,
        KERN_PROC_PID, getpid(), &count);

    if (!proc || count != 1) {
        procstat_close(stat);
        fprintf(stderr, "Unable to getprocs\n");
        exit(1);
    }

    struct kinfo_vmentry *map = procstat_getvmmap(stat,
        proc, &count);

    if (!map || count == 0) {
        procstat_freeprocs(stat, proc);
        procstat_close(stat);
        fprintf(stderr, "Unable to get vmmap\n");
        exit(1);
    }

    for (int i = 0; i < count; i++) {
        add_mapping(map[i].kve_start, map[i].kve_end,
            kvme_to_prot(map[i].kve_protection),
            kvme_to_flags(map[i].kve_flags),
            kvme_to_type(map[i].kve_type), map[i].kve_path);
    }

    procstat_freevmmap(stat, map);
    procstat_freeprocs(stat, proc);
    procstat_close(stat);
}
#endif

void load_mappings_run_procstat()
{
    char buffer[1024];
    FILE *fp;

    sprintf(buffer, "procstat -v %d", getpid());
    
    fp = popen(buffer, "r");

    if (fp == NULL) {
        fprintf(stderr, "Unable to run procstat\n");
        exit(1);
    }

    while (fgets(buffer, sizeof(buffer), fp)) {
        char prot[6], flags[6], type[3], path[PATH_MAX];
        uintptr_t start, end;

        strcpy(path, "");
        int rc = sscanf(buffer, "%*d %" PRIxPTR " %" PRIxPTR
            " %5s %*d %*d %*d %*d %5s %2s %s", &start, &end, prot, flags, type, path);

        if (rc >= 5)
            add_mapping(start, end, str_to_prot(prot),
                str_to_flags(flags), str_to_type(type), path);
    }

    pclose(fp);
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

    struct mapping *mapping = (struct mapping *)vec_alloc(v, 1);
    if (!mapping) return 0;

    mapping->start = start;
    mapping->end = end;
    mapping->prot = str_to_prot(prot);
    mapping->flags = str_to_flags(flags);
    mapping->type = str_to_type(type);

    cp = strrchr(path, '/');
    mapping->pathstr = string_alloc(path);
    mapping->namestr = string_alloc(cp ? cp+1 : path);

    for (int i = 0; i < vec_getcount(v); i++) {
        struct mapping *mp = (struct mapping *)vec_get(v, i);
        if (!strcmp(path, string_get(mp->pathstr))) {
            mapping->base = mapping - mp;
            break;
        }
    }

    if (!mapping->base && *path)
        load_symbols(&mapping->symbols, path);

    return 1;
}


void load_mappings()
{
    struct vec mappings;
    char cmd[2048];

    sprintf(cmd, "procstat -v %d", getpid());
    vec_init(&mappings, sizeof(struct mapping), 1000);

    if (!load_array_from_cmd(cmd, load_mapping_procstat, &mappings)) {
        fprintf(stderr, "Unable to load mappings");
        exit(1);        
    }

printf("---\n");
    for (int i = 0; i < vec_getcount(&mappings); i++)
        print_mapping((struct mapping *)vec_get(&mappings, i));
printf("---\n");

    load_mappings_run_procstat();
}


struct mapping *find_mapping(uintptr_t addr)
{
    for (int i = 0; i < nmappings; i++) {
        if (addr >= mappings[i].end) continue;
        if (addr < mappings[i].start) break;
        return &mappings[i];
    }

    load_mappings();

    for (int i = 0; i < nmappings; i++) {
        if (addr >= mappings[i].end) continue;
        if (addr < mappings[i].start) break;
        return &mappings[i];
    }

    return NULL;
}


void print_mappings()
{
    int i;

    load_mappings();

    for (i = 0; i < nmappings; i++)
        if (mappings[i].prot != CT_PROT_NONE)
            print_mapping(&mappings[i]);
}


int check_address_valid(void ***pptr)
{
    uintptr_t addr = (uintptr_t)*pptr;

    for (int i = 0; i < nmappings; i++) {
        if (addr >= mappings[i].end) continue;
        if (addr < mappings[i].start) break;

        if (mappings[i].prot == CT_PROT_NONE) {
            *(char **)pptr += (mappings[i].end - sizeof(void *)) - (size_t)*pptr;
            return 0;
        }

        return 1;
    }

    return 0;
}


char *mapping_getname(struct mapping *mapping)
{
    return string_get(mapping->namestr);
}


char *mapping_getpath(struct mapping *mapping)
{
    return string_get(mapping->pathstr);
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
