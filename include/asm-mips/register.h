/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by Ralf Baechle
 */
#ifndef __ASM_MIPS_REGISTER_H
#define __ASM_MIPS_REGISTER_H

/*
 * Types to represent a register
 */
#ifdef __mips64
/*
 * The R4000 port makes use of the 64 bit wide registers
 */
#ifndef __LANGUAGE_ASSEMBLY__
typedef long long __register_t;
typedef unsigned long long __uregister_t;
#endif

#else

/*
 * Good 'ole R3000 has only 32 bit wide registers
 */
#ifndef __LANGUAGE_ASSEMBLY__
typedef long __register_t;
typedef unsigned long __uregister_t;
#endif
#endif

#endif /* __ASM_MIPS_REGISTER_H */ 
