/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_UTIL_H_
#define _CHERITREE_UTIL_H_

#include <stddef.h>


/*
 *  Linear vector, grown on demand.
 */
struct vec {
    char *addr;         // Array of elements
    int count;          // Elements in use
    int maxcount;       // Elements allocated
    size_t size;        // Element size
    int expect;         // Allocation count
};

void cheritree_vec_init(struct vec *v, size_t size, int expect);
void *cheritree_vec_alloc(struct vec *v, int n);
void *cheritree_vec_get(const struct vec *v, int index);
void cheritree_vec_trim(struct vec *v);
void cheritree_vec_delete(struct vec *v);


/*
 *  String store, grown on demand.
 */
typedef int string_t;

string_t cheritree_string_alloc(const char *s);
const char *cheritree_string_get(string_t s);


/*
 *  Access functions.
 */
#define getcount(v)     (v)->count
#define getpath(x)      ((x) ? cheritree_string_get((x)->pathstr) : "")
#define getname(x)      ((x) ? cheritree_string_get((x)->namestr) : "")
#define setname(x,s)    (x)->namestr = cheritree_string_alloc((s))
#define setpath(x,s)    (x)->pathstr = cheritree_string_alloc((s))


/*
 *  Load array from command or path.
 */
int cheritree_load_from_cmd(const char *cmd,
    int (loadelement)(char *line, struct vec *v), struct vec *v);

int cheritree_load_from_path(const char *path,
    int (loadelement)(char *line, struct vec *v), struct vec *v);

#endif /* _CHERITREE_UTIL_H_ */
