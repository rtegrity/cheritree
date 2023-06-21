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
#include "symbol.h"
#include "mapping.h"
#include "util.h"


struct image {
    struct vec symbols;
    string_t pathstr;
};

static struct vec images;


static struct image *find_image(const char *path)
{
    for (int i = 0; i < getcount(&images); i++) {
        struct image *image = getimage(&images, i);

        if (!strcmp(getpath(image), path))
            return image;
    }

    return NULL;
}


static void print_symbol(struct symbol *symbol)
{
    printf("%#" PRIxPTR " %c %s\n", symbol->value,
        symbol->type, getname(symbol));
}


void print_symbols(const char *path)
{
    struct image *image = find_image(path);

    for (int i = 0; image && i < getcount(&image->symbols); i++)
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

    if (!*path) return;
    if (find_image(path)) return;

    struct image *image = (struct image *)vec_alloc(&images, 1);

    sprintf(cmd, "nm -ne %s", path);
    vec_init(&image->symbols, sizeof(struct symbol), 1024);

    if (!load_array_from_cmd(cmd, load_symbol, &image->symbols)) {
        fprintf(stderr, "Unable to load symbols");
        exit(1);
    }
}


struct symbol *
find_symbol(struct mapping *mapping, uintptr_t addr)
{
    struct image *image = find_image(getpath(mapping));
    struct mapping *base = &mapping[mapping->base];
    int i = 0;

    if (!image) return NULL;

    for (; i < getcount(&image->symbols); i++) {
        struct symbol *sym = getsymbol(&image->symbols, i);
        if (sym && base->start + (size_t)sym->value > addr) break;
    }

    return (i) ? getsymbol(&image->symbols, i) : NULL;
}
