/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#ifndef _CHERITREE_UTIL_H_
#define _CHERITREE_UTIL_H_

#include <stddef.h>


void *load_array_from_cmd(const char *cmd,
    int (loadelement)(char *line, void *element, void *context),
    void *context, size_t size, int *pcount, int expect);

void *load_array_from_path(const char *path,
    int (loadelement)(char *line, void *element, void *context),
    void *context, size_t size, int *pcount, int expect);


#endif /* _CHERITREE_UTIL_H_ */
