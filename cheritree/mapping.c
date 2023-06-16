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
#include "module.h"
#include "mapping.h"


extern int saveregs(void **);

static int nmappings;
static struct mapping mappings[4096];

static int str_to_prot(const char *str);
static void prot_to_str(char *str, int prot);
static int str_to_type(const char *str);
static void type_to_str(char *str, int type);


void print_mapping(struct mapping *mapping)
{
    printf("%#" PRIxPTR "-%#" PRIxPTR " %s %s %s %s\n",
        mapping->start, mapping->end, mapping->_prot,
        mapping->flags, mapping->type,
        (*mapping->module->path ? mapping->module->path : mapping->module->name));
}


void add_mapping(uintptr_t start, uintptr_t end,
    char *prot, char *flags, char *type, char *path)
{
    int i = 0;

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
    strcpy(mappings[i]._prot, prot);
    strcpy(mappings[i].flags, flags);
    strcpy(mappings[i].type, type);
    mappings[i].guard = !strcmp(prot, "-----");
    mappings[i].module = add_module(path, start);
}


void load_mappings()
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
        char prot[6], flags[6], type[3];

        prot_to_str(prot, kvme_to_prot(map[i].kve_protection));
        type_to_str(type, kvme_to_type(map[i].kve_type));

        flags[0] = (map[i].kve_flags & KVME_FLAG_COW) ? 'C' :
                    (map[i].kve_flags & KVME_FLAG_GUARD) ? 'G' :
                    (map[i].kve_flags & KVME_FLAG_UNMAPPED) ? 'U' : '-';
        flags[1] = (map[i].kve_flags & KVME_FLAG_NEEDS_COPY) ? 'N' : '-';
        flags[2] = (map[i].kve_flags & KVME_FLAG_SUPER) ? 'S' : '-';
        flags[3] = (map[i].kve_flags & KVME_FLAG_GROWS_UP) ? 'U' :
                    (map[i].kve_flags & KVME_FLAG_GROWS_DOWN) ? 'D' : '-';
        flags[4] = (map[i].kve_flags & KVME_FLAG_USER_WIRED) ? 'W' : '-';
        flags[5] = 0;

        add_mapping(map[i].kve_start, map[i].kve_end,
            prot, flags, type, map[i].kve_path);
    }

    procstat_freevmmap(stat, map);
    procstat_freeprocs(stat, proc);
    procstat_close(stat);
    return;
}


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
            add_mapping(start, end, prot, flags, type, path);
    }

    fclose(fp);
}


struct mapping *find_mapping(uintptr_t addr)
{
    static struct module unknown_module = { 0, "Unknown", "Unknown" };
    static struct mapping unknown_mapping = {
        0, 0, "", "", "", 0, &unknown_module
    };

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

    return &unknown_mapping;
}


void print_mappings()
{
    int i;

    load_mappings();

    for (i = 0; i < nmappings; i++)
        if (!mappings[i].guard)
            print_mapping(&mappings[i]);
}


int check_address_valid(void ***pptr)
{
    uintptr_t addr = (uintptr_t)*pptr;

    for (int i = 0; i < nmappings; i++) {
        if (addr >= mappings[i].end) continue;
        if (addr < mappings[i].start) break;

        if (mappings[i].guard) {
            *(char **)pptr += (mappings[i].end - sizeof(void *)) - (size_t)*pptr;
            return 0;
        }

        return 1;
    }

    return 0;
}


static int str_to_prot(const char *str)
{
    return (str[0] == 'r' ? CT_PROT_READ : 0)
         | (str[1] == 'w' ? CT_PROT_WRITE : 0)
         | (str[2] == 'x' ? CT_PROT_EXEC : 0)
         | (str[3] == 'R' ? CT_PROT_READ_CAP : 0)
         | (str[3] == 'W' ? CT_PROT_WRITE_CAP : 0);
}


static void prot_to_str(char *str, int prot)
{
    str[0] = (prot & CT_PROT_READ) ? 'r' : '-';
    str[1] = (prot & CT_PROT_WRITE) ? 'w' : '-';
    str[2] = (prot & CT_PROT_EXEC) ? 'x' : '-';
    str[3] = (prot & CT_PROT_READ_CAP) ? 'R' : '-';
    str[4] = (prot & CT_PROT_WRITE_CAP) ? 'W' : '-';
    str[5] = 0;
}


static int str_to_type(const char *str)
{
    return !strcmp(str, "--") ? CT_TYPE_NONE :
           !strcmp(str, "df") ? CT_TYPE_DEFAULT :
           !strcmp(str, "vn") ? CT_TYPE_VNODE :
           !strcmp(str, "sw") ? CT_TYPE_SWAP :
           !strcmp(str, "dv") ? CT_TYPE_DEVICE :
           !strcmp(str, "ph") ? CT_TYPE_PHYS :
           !strcmp(str, "dd") ? CT_TYPE_DEAD :
           !strcmp(str, "sg") ? CT_TYPE_SG :
           !strcmp(str, "md") ? CT_TYPE_MGTDEVICE :
           !strcmp(str, "gd") ? CT_TYPE_GUARD : CT_TYPE_UNKNOWN;
}


static void type_to_str(char *str, int type)
{
    strcpy(str, (type == CT_TYPE_NONE) ? "--" :
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
