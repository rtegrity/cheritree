/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_UTIL_H_
#define _CHERITREE_UTIL_H_

#include <stddef.h>


/*
 *  Namespace definitions
 */
#define vec_init        cheritree_vec_init
#define vec_alloc       cheritree_vec_alloc
#define vec_get         cheritree_vec_get
#define vec_trim        cheritree_vec_trim
#define vec_delete      cheritree_vec_delete

#define string_get      cheritree_string_get
#define string_alloc    cheritree_string_alloc

#define load_array_from_cmd     cheritree_load_array_from_cmd
#define load_array_from_path    cheritree_load_array_from_path


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

void vec_init(struct vec *v, size_t size, int expect);
void *vec_alloc(struct vec *v, int n);
void *vec_get(const struct vec *v, int index);
void vec_trim(struct vec *v);
void vec_delete(struct vec *v);


/*
 *  String store, grown on demand.
 */
typedef int string_t;

string_t string_alloc(const char *s);
const char *string_get(string_t s);


/*
 *  Access functions.
 */
#define getcount(v)     (v)->count
#define getpath(x)      string_get((x)->pathstr)
#define getname(x)      string_get((x)->namestr)
#define setname(x,s)    (x)->namestr = string_alloc((s))
#define setpath(x,s)    (x)->pathstr = string_alloc((s))


/*
 *  Load array from command or path.
 */
int load_array_from_cmd(const char *cmd,
    int (loadelement)(char *line, struct vec *v), struct vec *v);

int load_array_from_path(const char *path,
    int (loadelement)(char *line, struct vec *v), struct vec *v);

#endif /* _CHERITREE_UTIL_H_ */
