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
    char *addr;
    int count;
    int maxcount;
    size_t size;
    int expect;
};

void vec_init(struct vec *v, size_t size, int expect);
void *vec_alloc(struct vec *v, int n);
int vec_getcount(struct vec *v);
void *vec_get(struct vec *v, int index);
void vec_trim(struct vec *v);
void vec_delete(struct vec *v);


/*
 *  String store, grown on demand.
 */
int string_alloc(const char *s);
char *string_get(int offset);


int load_array_from_cmd(const char *cmd,
    int (loadelement)(char *line, struct vec *v), struct vec *v);

int load_array_from_path(const char *path,
    int (loadelement)(char *line, struct vec *v), struct vec *v);


#endif /* _CHERITREE_UTIL_H_ */
