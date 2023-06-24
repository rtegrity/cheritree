/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include "symbol.h"
#include "mapping.h"
#include "util.h"


static struct vec images;


static image_t *find_image(const char *path)
{
    int i;

    if (!path || !*path) return NULL;

    for (i = 0; i < getcount(&images); i++) {
        image_t *image = getimage(&images, i);

        if (!strcmp(getpath(image), path))
            return image;
    }

    return NULL;
}


static void print_symbol(const symbol_t *symbol)
{
    printf("%#" PRIxADDR " %c %s\n", symbol->value,
        symbol->type, getname(symbol));
}


void cheritree_print_symbols(const char *path)
{
    const image_t *image = find_image(path);
    int i;

    if (!image) return;

    for (i = 0; i < getcount(&image->symbols); i++)
        print_symbol(getsymbol(&image->symbols, i));
}


static int load_symbol(char *buffer, struct vec *v)
{
    char type[2], name[1024];
    addr_t value;

    if (sscanf(buffer, "%" PRIxADDR " %1s %1023s", &value, type, name) != 3)
        return 1;

    symbol_t *symbol = (symbol_t *)cheritree_vec_alloc(v, 1);
    setname(symbol, name);
    symbol->value = value;
    symbol->type = type[0];
    return 1;
}


void cheritree_load_symbols(const char *path)
{
    char cmd[2048];
 
    if (images.addr == 0)
        cheritree_vec_init(&images, sizeof(image_t), 1024);

    if (!path || !*path) return;
    if (find_image(path)) return;

    image_t *image = (image_t *)cheritree_vec_alloc(&images, 1);

    cheritree_vec_init(&image->symbols, sizeof(symbol_t), 1024);
    setpath(image, path);

    sprintf(cmd, "nm -ne %s 2>/dev/null", path);
    if (cheritree_load_from_cmd(cmd, load_symbol, &image->symbols)) return;

    // Retry with dynamic symbols
    sprintf(cmd, "nm -Dne %s", path);
    if (cheritree_load_from_cmd(cmd, load_symbol, &image->symbols)) return;

    fprintf(stderr, "Unable to load symbols");
    exit(1);
}


const char *cheritree_find_type(const char *path,
    addr_t base, addr_t start, addr_t end)
{
    const image_t *image = find_image(path);
    int i;

    if (!image) return NULL;

    for (i = 0; i < getcount(&image->symbols); i++) {
        const symbol_t *sym = getsymbol(&image->symbols, i);
        addr_t addr = base + sym->value;

        if (addr > end) break;

        if (start <= addr && addr < end) {
            if (strchr("Tt", sym->type)) return "text";
            if (strchr("BCb", sym->type)) return "bss";
            if (strchr("DRVdr", sym->type)) return "data";
        }
    }

    return NULL;
}


symbol_t *cheritree_find_symbol(const char *path,
    addr_t base, addr_t addr)
{
    const image_t *image = find_image(path);
    int i;

    if (!image) return NULL;

    for (i = 0; i < getcount(&image->symbols); i++) {
        const symbol_t *sym = getsymbol(&image->symbols, i);
        if (base + sym->value > addr) break;
    }

    return (i) ? getsymbol(&image->symbols, i-1) : NULL;
}
