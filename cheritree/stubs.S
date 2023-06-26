/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <machine/asm.h>

ENTRY(cheritree_print_capabilities)
	/* Preserve a large area of the stack */
	sub csp, csp, #0x100, lsl #12
	stp c29, c30, [csp, #-(CAP_WIDTH * 2)]!
	stp c29, c30, [csp, #(CAP_WIDTH * 31)]
	mov c29, csp

   	/* Save the registers */
	stp c0, c1, [csp, #(CAP_WIDTH * 2)]
	stp	c2, c3, [csp, #(CAP_WIDTH * 4)]
	stp	c4, c5, [csp, #(CAP_WIDTH * 6)]
	stp	c6, c7, [csp, #(CAP_WIDTH * 8)]
	stp	c8, c9, [csp, #(CAP_WIDTH * 10)]
	stp	c10, c11, [csp, #(CAP_WIDTH * 12)]
	stp	c12, c13, [csp, #(CAP_WIDTH * 14)]
	stp	c14, c15, [csp, #(CAP_WIDTH * 16)]
	stp	c16, c17, [csp, #(CAP_WIDTH * 18)]
	stp	c18, c19, [csp, #(CAP_WIDTH * 20)]
	stp	c20, c21, [csp, #(CAP_WIDTH * 22)]
	stp	c22, c23, [csp, #(CAP_WIDTH * 24)]
	stp	c24, c25, [csp, #(CAP_WIDTH * 26)]
	stp	c26, c27, [csp, #(CAP_WIDTH * 28)]
	str	C28, [csp, #(CAP_WIDTH * 30)]
	str	C31, [csp, #(CAP_WIDTH * 33)]
	mrs c10, ddc
   	str c10, [csp, #(CAP_WIDTH * 34)]

	add c0, csp, #(CAP_WIDTH * 2)
	mov w1, #33
	bl _cheritree_print_capabilities

	ldp c29, c30, [csp], #(CAP_WIDTH * 2)
	add csp, csp, #0x100, lsl #12
	ret
END(cheritree_print_capabilities)
