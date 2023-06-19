/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"


static void *load_array(FILE *fp,
    int (loadelement)(char *line, void *element, void *context),
    void *context, size_t size, int *pcount, int expect)
{
    int maxcount = 0, count = 0;
    char *array = NULL;

    while (fp != NULL) {
        char buffer[2048];

        if (!array || count >= maxcount) {
            char *ap = realloc(array, (maxcount + expect) * size);

            if (ap == NULL) {
                free(array);
                array = NULL;
                break;
            }

            array = ap;
            maxcount += expect;
        }

        if (fgets(buffer, sizeof(buffer), fp) == NULL)
            break;

        memset(array + count * size, 0, size);
        if (loadelement(buffer, array + count * size, context)) count++;
    }

    if (pcount) *pcount = (array) ? count : 0;

    if (array && count < maxcount) {
        char *ap = realloc(array, count * size);
        if (ap) return ap;
    }

    return array;
}


/*
 *  Load array from command
 */
void *load_array_from_cmd(const char *cmd,
    int (loadelement)(char *line, void *element, void *context),
    void *context, size_t size, int *pcount, int expect)
{
    FILE *fp = popen(cmd, "r");
    void *array = load_array(fp, loadelement, context, size, pcount, expect);

    if (fp) pclose(fp);
    return array;
}


/*
 *  Load array from path
 */
void *load_array_from_path(const char *path,
    int (loadelement)(char *line, void *element, void *context),
    void *context, size_t size, int *pcount, int expect)
{
    FILE *fp = fopen(path, "r");
    void *array = load_array(fp, loadelement, context, size, pcount, expect);

    if (fp) fclose(fp);
    return array;
}
