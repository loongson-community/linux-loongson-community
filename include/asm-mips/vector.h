/*
 * include/asm-mips/vector.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995 by Ralf Baechle
 */

/*
 * This structure defines how to access various features of
 * different machine types and how to access them.
 *
 * FIXME: More things need to be accessed via this vector.
 */
struct feature {
	void (*handle_int)(void);
};

/*
 * Similar to the above this is a structure that describes various
 * CPU dependend features.
 *
 * FIXME: This vector isn't being used yet
 */
struct cpu {
	int dummy;	/* keep GCC from complaining */
};

extern struct feature *feature;
extern struct cpu *cpu;
