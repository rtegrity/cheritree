/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <machine/asm.h>

ENTRY(cheritree_saveregs)
	/* Save the registers */
	stp	c0, c1, [c0], #(CAP_WIDTH * 2)
	stp	c2, c3, [c0], #(CAP_WIDTH * 2)
	stp	c4, c5, [c0], #(CAP_WIDTH * 2)
	stp	c6, c7, [c0], #(CAP_WIDTH * 2)
	stp	c8, c9, [c0], #(CAP_WIDTH * 2)
	stp	c10, c11, [c0], #(CAP_WIDTH * 2)
	stp	c12, c13, [c0], #(CAP_WIDTH * 2)
	stp	c14, c15, [c0], #(CAP_WIDTH * 2)
	stp	c16, c17, [c0], #(CAP_WIDTH * 2)
	stp	c18, c19, [c0], #(CAP_WIDTH * 2)
	stp	c20, c21, [c0], #(CAP_WIDTH * 2)
	stp	c22, c23, [c0], #(CAP_WIDTH * 2)
	stp	c24, c25, [c0], #(CAP_WIDTH * 2)
	stp	c26, c27, [c0], #(CAP_WIDTH * 2)
	stp	c28, C29, [c0], #(CAP_WIDTH * 2)
	stp	c30, C31, [c0], #(CAP_WIDTH * 2)
	ret
END(cheritree_saveregs)
