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
#include "module.h"
#include "mapping.h"


extern int saveregs(void **);

static int nmappings;
static struct mapping mappings[4096];


void print_mapping(struct mapping *mapping)
{
    printf("%#" PRIxPTR "-%#" PRIxPTR " %s %s %s %s\n",
        mapping->start, mapping->end, mapping->prot,
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
    strcpy(mappings[i].prot, prot);
    strcpy(mappings[i].flags, flags);
    strcpy(mappings[i].type, type);
    mappings[i].guard = !strcmp(prot, "-----");
    mappings[i].module = add_module(path, start);
}


void load_mappings()
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