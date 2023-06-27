/*-
 *  SPDX-License-Identifier: BSD-3-Clause
 *
 *  Copyright (c) 2023, rtegrity ltd. All rights reserved.
 */

#include <machine/asm.h>

ENTRY(cheritree_print_capabilities)
	/* Preserve a large area of the stack */
	stp c29, c30, [csp, #-48]!
	str c28, [csp, #32]
	mov c29, csp
	sub csp, csp, #0x100, lsl #12
	sub csp, csp, #0x3d0
	alignd csp, csp, #9

	.cfi_def_cfa c29, 48
	.cfi_offset c28, -16
	.cfi_offset c30, -32
	.cfi_offset c29, -48

   	/* Save the registers */
	stp c0, c1, [csp, #(CAP_WIDTH)]
	stp	c2, c3, [csp, #(CAP_WIDTH * 2)]
	stp	c4, c5, [csp, #(CAP_WIDTH * 4)]
	stp	c6, c7, [csp, #(CAP_WIDTH * 6)]
	stp	c8, c9, [csp, #(CAP_WIDTH * 8)]
	stp	c10, c11, [csp, #(CAP_WIDTH * 10)]
	stp	c12, c13, [csp, #(CAP_WIDTH * 12)]
	stp	c14, c15, [csp, #(CAP_WIDTH * 14)]
	stp	c16, c17, [csp, #(CAP_WIDTH * 16)]
	stp	c18, c19, [csp, #(CAP_WIDTH * 18)]
	stp	c20, c21, [csp, #(CAP_WIDTH * 20)]
	stp	c22, c23, [csp, #(CAP_WIDTH * 22)]
	stp	c24, c25, [csp, #(CAP_WIDTH * 24)]
	stp	c26, c27, [csp, #(CAP_WIDTH * 26)]
	ldp c0, c1, [c29]
	stp	c28, c0, [csp, #(CAP_WIDTH * 28)]
	mrs c0, ddc
	stp	c1, c0, [csp, #(CAP_WIDTH * 30)]

	/* Print capabilities */
	mov c0, csp
	mov w1, #32
	bl _cheritree_print_capabilities

   	/* Restore all registers */
	ldp c0, c1, [csp, #(CAP_WIDTH)]
	ldp	c2, c3, [csp, #(CAP_WIDTH * 2)]
	ldp	c4, c5, [csp, #(CAP_WIDTH * 4)]
	ldp	c6, c7, [csp, #(CAP_WIDTH * 6)]
	ldp	c8, c9, [csp, #(CAP_WIDTH * 8)]
	ldp	c10, c11, [csp, #(CAP_WIDTH * 10)]
	ldp	c12, c13, [csp, #(CAP_WIDTH * 12)]
	ldp	c14, c15, [csp, #(CAP_WIDTH * 14)]
	ldp	c16, c17, [csp, #(CAP_WIDTH * 16)]
	ldp	c18, c19, [csp, #(CAP_WIDTH * 18)]
	ldp	c20, c21, [csp, #(CAP_WIDTH * 20)]
	ldp	c22, c23, [csp, #(CAP_WIDTH * 22)]
	ldp	c24, c25, [csp, #(CAP_WIDTH * 24)]
	ldp	c26, c27, [csp, #(CAP_WIDTH * 26)]

	mov csp, c29
	ldr c28, [csp, #32]
	ldp c29, c30, [csp], #48
	ret
END(cheritree_print_capabilities)
