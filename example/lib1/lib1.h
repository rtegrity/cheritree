/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <stdlib.h>

extern int lib1_int;

extern int lib1_init();
extern int lib1_access_int();
extern int lib1_local_access_int();
extern int lib1_check_string_arg(const char *, size_t);

typedef int (*int_func_t)();
extern int_func_t lib1_access_int_function();