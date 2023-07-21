/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"


static vec_t strings;


/*
 *  Load vec from FILE handle
 */
static int load_vec(FILE *fp,
    int (loadelement)(char *line, vec_t *v), vec_t *v)
{
    while (fp != NULL) {
        char buffer[2048];

        if (fgets(buffer, sizeof(buffer), fp) == NULL)
            break;

        if (!loadelement(buffer, v))
            break;
    }

    cheritree_vec_trim(v);
    return (v->addr != NULL);
}


/*
 *  Load from command
 */
int cheritree_load_from_cmd(const char *cmd,
    int (loadelement)(char *line, vec_t *v), vec_t *v)
{
    FILE *fp = popen(cmd, "r");
    int rc = load_vec(fp, loadelement, v);

    if (fp) pclose(fp);
    return rc;
}


/*
 *  Load from path
 */
int cheritree_load_from_path(const char *path,
    int (loadelement)(char *line, vec_t *v), vec_t *v)
{
    FILE *fp = fopen(path, "r");
    int rc = load_vec(fp, loadelement, v);

    if (fp) fclose(fp);
    return rc;
}


/*
 *  Map of address ranges, grown on demand.
 *
 *  Note: Integers are used instead of pointers to minimise the
 *  number of capabilities introduced.
 */
void cheritree_map_init(map_t *v, int expect)
{
    cheritree_vec_init(v, sizeof(range_t), expect);
}


int cheritree_map_add(map_t *v, addr_t start, addr_t end)
{
    range_t *range = (range_t *)v->addr;
    int i, j;

    for (i = 0; i < v->count; i++) {
        if (range[i].start <= start && end <= range[i].end) return 0;
        if (start <= range[i].end) break;
    }

    if (i < v->count && end >= range[i].start) {
        if (start < range[i].start)
            range[i].start = start;

        if (end > range[i].end)
            range[i].end = end;

        for (j = i; j < v->count && end >= range[j].start; j++)
            if (range[j].end > end)
                range[i].end = range[j].end;

        if (j > i + 1) {
            if (j < v->count)
                memmove(&range[i+1], &range[j], (v->count-j) * sizeof(range_t));
            v->count -= (j-i-1);
        }

        return 1;
    }

    cheritree_vec_alloc(v, 1);
    range = (range_t *)v->addr;

    if (i < v->count - 1)
        memmove(&range[i+1], &range[i], (v->count-i-1) * sizeof(range_t));

    range[i].start = start;
    range[i].end = end;
    return 1;
}


int cheritree_map_find(map_t *v, addr_t addr, range_t *prange)
{
    range_t *range = (range_t *)v->addr;
    int i;

    for (i = 0; i < v->count; i++) {
        if (range[i].start <= addr && addr < range[i].end) {
            *prange = range[i];
            return 1;
        }
        
        if (addr <= range[i].end) break;
    }

    return 0;
}


void cheritree_map_print(map_t *v)
{
    range_t *range = (range_t *)v->addr;
    int i;

    printf("Map at %p with %d entries:\n", v, getcount(v));

    for (i = 0; i < getcount(v); i++)
        printf("%" PRIxADDR "-%" PRIxADDR "\n", range[i].start, range[i].end);
}


void cheritree_map_reset(map_t *v)
{
    v->count = 0;
}


void cheritree_map_delete(map_t *v)
{
    cheritree_vec_delete(v);
}


/*
 *  String store, grown on demand.
 *
 *  Note: Strings are referenced by offset to minimise the number
 *  of capabilities introduced. There is no need to support deletion
 *  since the address space is assumed to be relatively static.
 */
string_t cheritree_string_alloc(const char *s)
{
    char *addr;

    if (!s || !*s) return 0;

    if (!strings.addr)
        cheritree_vec_init(&strings, sizeof(char), 64 * 1024);
 
    addr = cheritree_vec_alloc(&strings, strlen(s) + 1);

    return (string_t)(strcpy(addr, s) - strings.addr + 1);
}


const char *cheritree_string_get(string_t s)
{
    return (s) ? strings.addr + s - 1 : "";
}


/*
 *  Linear vector, grown on demand.
 *
 *  Note: A linear vector is used instead of more flexible
 *  structures to minimise the number of capabilities introduced.
 *  Memory allocation failures will result in the program exiting.
 */
void cheritree_vec_init(vec_t *v, size_t size, int expect)
{
    v->addr = NULL;
    v->size = size;
    v->expect = expect;
    v->count = 0;
    v->maxcount = 0;
}


void *cheritree_vec_alloc(vec_t *v, int n)
{
    char *addr;

    if (!v->addr || v->count + n > v->maxcount) {
        int alloc = (n > v->expect) ? n : v->expect;
        char *ap = realloc(v->addr, (v->maxcount + alloc) * v->size);

        if (ap == NULL) {
            fprintf(stderr, "CheriTree: Unable to allocate memory");
            exit(1);
        }

        v->addr = ap;
        v->maxcount += alloc;
    }

    addr = v->addr + v->count * v->size;
    v->count += n;

    memset(addr, 0, n * v->size);
    return addr;
}


void *cheritree_vec_get(const vec_t *v, int index)
{
    return (v->addr) ? v->addr + (index * v->size) : NULL;
}


void cheritree_vec_trim(vec_t *v)
{
    if (v->addr && v->count < v->maxcount) {
        char *ap = realloc(v->addr, v->count * v->size);

        if (ap == NULL) return;

        v->addr = ap;
        v->maxcount = v->count;
    }
}


void cheritree_vec_delete(vec_t *v)
{
#ifdef CHERITREE_DEBUG
    if (v->addr)
        memset(v->addr, 0x5a, v->maxcount * v->size);
#endif
    free(v->addr);
    v->addr = NULL;
    v->count = 0;
    v->maxcount = 0;
}
