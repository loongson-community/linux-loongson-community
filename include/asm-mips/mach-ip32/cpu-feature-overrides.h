/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2005 Ilya A. Volynets-Evenbakh
 */
#ifndef __ASM_MACH_IP32_CPU_FEATURE_OVERRIDES_H
#define __ASM_MACH_IP32_CPU_FEATURE_OVERRIDES_H

/*
 * R5000 has an interesting "restriction":  ll(d)/sc(d)
 * instructions to XKPHYS region simply do uncached bus
 * requests. This breaks all the atomic bitops functions.
 * so, for 64bit IP32 kernel we just don't use ll/sc.
 * This does not affect luserland.
 */
#if defined(CONFIG_CPU_R5000) && defined(CONFIG_MIPS64)
#define cpu_has_llsc	0
#endif

#endif /* __ASM_MACH_IP32_CPU_FEATURE_OVERRIDES_H */
