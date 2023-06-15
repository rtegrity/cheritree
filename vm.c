/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheriintrin.h>
#endif


extern int saveregs(void **);

static struct mapping {
    uintptr_t start;
    uintptr_t end;
    char prot[6];
    char flags[6];
    char type[3];
    char *path;
} mappings[4096];

static int nmappings;


void print_memory_mapping(struct mapping *mapping)
{
    printf("%#" PRIxPTR "-%#" PRIxPTR " %s %s %s %s\n",
        mapping->start, mapping->end, mapping->prot,
        mapping->flags, mapping->type, mapping->path);
}


void add_memory_mapping(uintptr_t start, uintptr_t end,
    char *prot, char *flags, char *type, char *path)
{
    int i = 0;

    for (; i < nmappings; i++) {
        if (start >= mappings[i].end) continue;

        if (start < mappings[i].start) {
            for (int j = nmappings; j > i; j--)
                mappings[j] = mappings[j-1];
        }

        break;
    }

    if (i >= sizeof(mappings) / sizeof(mappings[0])) {
        fprintf(stderr, "Too many memory mappings");
        exit(1);
    }

    mappings[i].start = start;
    mappings[i].end = end;
    strcpy(mappings[i].prot, prot);
    strcpy(mappings[i].flags, flags);
    strcpy(mappings[i].type, type);
    mappings[i].path = strdup(path);
    nmappings++;
}


void load_memory_mappings()
{
    char buffer[1024];
    FILE *fp;

    sprintf(buffer, "procstat -v %d", getpid());
    
    fp = popen(buffer, "r");

    while (fgets(buffer, sizeof(buffer), fp)) {
        char prot[6], flags[6], type[3], path[PATH_MAX];
        uintptr_t start, end;

        strcpy(path, "");
        int rc = sscanf(buffer, "%*d %" PRIxPTR " %" PRIxPTR
            " %5s %*d %*d %*d %*d %5s %2s %s", &start, &end, prot, flags, type, path);

        if (rc >= 5 && strcmp(prot, "-----") != 0)
            add_memory_mapping(start, end, prot, flags, type, path);
    }

    fclose(fp);
}


void print_memory_mappings()
{
    int i;

    load_memory_mappings();

    for (i = 0; i < nmappings; i++)
        print_memory_mapping(&mappings[i]);
}


struct mapping *find_memory_mapping(uintptr_t addr)
{
    static struct mapping unknown_mapping = {
        0, 0, "", "", "", "UNKNOWN"
    };

    for (int i = 0; i < nmappings; i++) {
        if (addr >= mappings[i].end) continue;
        if (addr < mappings[i].start) break;
        return &mappings[i];
    }

    load_memory_mappings();

    for (int i = 0; i < nmappings; i++) {
        if (addr >= mappings[i].end) continue;
        if (addr < mappings[i].start) break;
        return &mappings[i];
    }

    return &unknown_mapping;
}


void find_memory_references()
{
    void *regs[32];

    saveregs(regs);

    for (int i = 0; i < sizeof(regs) / sizeof(regs[0]); i++)
        if (cheri_is_valid(regs[i])) {
            uintptr_t addr = cheri_address_get(regs[i]);
            struct mapping *mapping = find_memory_mapping(addr);
            printf("c%d %#p    ", i, regs[i]);
            print_memory_mapping(mapping);
        }
}
