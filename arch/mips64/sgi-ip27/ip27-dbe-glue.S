/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 1999 by Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics
 *
 * Low level exception handling
 */
#define __ASSEMBLY__
#include <linux/init.h>
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/fpregdef.h>
#include <asm/mipsregs.h>
#include <asm/exception.h>

	BUILD_HANDLER ip27_ibe ip27_ibe cli silent		/* #6  */
	BUILD_HANDLER ip27_dbe ip27_dbe cli silent		/* #7  */
