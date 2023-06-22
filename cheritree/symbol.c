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


static struct image *find_image(const char *path)
{
    int i;

    if (!path || !*path) return NULL;

    for (i = 0; i < getcount(&images); i++) {
        struct image *image = getimage(&images, i);

        if (!strcmp(getpath(image), path))
            return image;
    }

    return NULL;
}


static void print_symbol(const struct symbol *symbol)
{
    printf("%#" PRIxPTR " %c %s\n", symbol->value,
        symbol->type, getname(symbol));
}


void print_symbols(const char *path)
{
    const struct image *image = find_image(path);
    int i;

    if (!image) return;

    for (i = 0; i < getcount(&image->symbols); i++)
        print_symbol(getsymbol(&image->symbols, i));
}


static int load_symbol(char *buffer, struct vec *v)
{
    char type[2], name[1024];
    uintptr_t value;

    if (sscanf(buffer, "%" PRIxPTR " %1s %1023s", &value, type, name) != 3)
        return 1;

    struct symbol *symbol = (struct symbol *)vec_alloc(v, 1);
    setname(symbol, name);
    symbol->value = value;
    symbol->type = type[0];
    return 1;
}


void load_symbols(const char *path)
{
    char cmd[2048];
 
    if (images.addr == 0)
        vec_init(&images, sizeof(struct image), 1024);

    if (!path || !*path) return;
    if (find_image(path)) return;

    struct image *image = (struct image *)vec_alloc(&images, 1);

    vec_init(&image->symbols, sizeof(struct symbol), 1024);
    setpath(image, path);
    sprintf(cmd, "nm -ne %s", path);

    if (!load_array_from_cmd(cmd, load_symbol, &image->symbols)) {
        fprintf(stderr, "Unable to load symbols");
        exit(1);
    }
}


const char *find_type(const struct mapping *mapping,
    uintptr_t start, uintptr_t end)
{
    const struct image *image = find_image(getpath(mapping));
    uintptr_t base = getbase(mapping);
    int i;

    if (!image) return NULL;

    for (i = 0; i < getcount(&image->symbols); i++) {
        const struct symbol *sym = getsymbol(&image->symbols, i);
        size_t addr = base + (size_t)sym->value;

        if (addr > end) break;

        if (start <= addr && addr < end) {
            if (strchr("Tt", sym->type)) return "text";
            if (strchr("BCb", sym->type)) return "bss";
            if (strchr("DRVdr", sym->type)) return "data";
        }
    }

    return NULL;
}


struct symbol *find_symbol(const struct mapping *mapping, uintptr_t addr)
{
    const struct image *image = find_image(getpath(mapping));
    uintptr_t base = getbase(mapping);
    int i;

    if (!image) return NULL;

    for (i = 0; i < getcount(&image->symbols); i++) {
        const struct symbol *sym = getsymbol(&image->symbols, i);
        if (base + (size_t)sym->value > addr) break;
    }

    return (i) ? getsymbol(&image->symbols, i-1) : NULL;
}

