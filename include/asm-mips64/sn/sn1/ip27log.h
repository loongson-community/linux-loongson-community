/* $Id$
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1992 - 1997, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2000 by Colin Ngam
 */

#ifndef	_ASM_SN_SN1_IP27LOG_H
#define	_ASM_SN_SN1_IP27LOG_H

#include <asm/sn/promlog.h>

/* Put the IP35 ip27log in the second EPROM chip */
#define IP27LOG_BASE(nasid)	((void *)(nasid < 0 || nasid == get_nasid() ? \
					LBOOT_BASE + LBOOT_STRIDE : \
					NODE_RBOOT_BASE(nasid) + LBOOT_STRIDE))

#define	IP27LOG_SEVERITY_SHFT	0
#define	IP27LOG_SEVERITY_MASK	(0xffff << IP27LOG_SEVERITY_SHFT)
#define	IP27LOG_FLAG_SHFT	16
#define	IP27LOG_FLAG_MASK	(0xffff << IP27LOG_FLAG_SHFT)

#define IP27LOG_INFO		0
#define IP27LOG_ERROR		1
#define IP27LOG_FATAL		2

#define	IP27LOG_FLAG_DUP	(1 << IP27LOG_FLAG_SHFT)

/*
 * NB: The key string for the log includes the key and a duplicate
 * delimiter. The size of this concatenated string shouldn't exceed
 * PROMLOG_KEY_MAX
 */

#define	IP27LOG_INFO_KEY	"Info"
#define	IP27LOG_ERROR_KEY	"Error"
#define	IP27LOG_FATAL_KEY	"Fatal"

#define	IP27LOG_DUP_DLM		"LOG_DUPL: "

#define IP27LOG_DO_INIT		(1 << 0)
#define IP27LOG_FOR_WRITING	(1 << 1)
#define IP27LOG_VERBOSE		(1 << 2)
#define IP27LOG_MY_FPROM_T	(1 << 3)
#define IP27LOG_DO_CLEAR	(1 << 4)

int	ip27log_setup(promlog_t *l, nasid_t nasid, int flags);

int	ip27log_setenv(nasid_t nasid,
		       char *key, char *value, int flags);

int	ip27log_getenv(nasid_t nasid,
		       char *key, char *value, char *defl, int flags);

int	ip27log_unsetenv(nasid_t nasid,
			 char *key, int flags);

#ifdef _STANDALONE
void	ip27log_printf(int severity, const char *fmt, ...);
#else
/* The kernel doesn't like this const */
void	ip27log_printf(int severity, char *fmt, ...);
void	ip27log_panic_printf(int severity, char *fmt, va_list ap);
#endif

#ifdef _STANDALONE
void	ip27log_printf_nasid(nasid_t nasid, int severity, const char *fmt, ...);
#else
/* The kernel doesn't like this const */
void	ip27log_printf_nasid(nasid_t nasid, int severity, char *fmt, ...);
#endif


#define	IP27LOG_MODULE_KEY	"LastModule"
#define	IP27LOG_PARTITION	"PartitionID"

#define	DISABLE_MEM_MASK	"DisableMemMask"
#define	DISABLE_MEM_SIZE	"DisableMemSz"
#define	DISABLE_MEM_REASON	"MemDisReason"

#define	SWAP_BANK		"SwapBank"
#define	FORCE_CONSOLE		"ForceConsole"

#define	DISABLE_CPU_A		"DisableA"
#define	DISABLE_CPU_B		"DisableB"
#define	DISABLE_CPU_C		"DisableC"
#define	DISABLE_CPU_D		"DisableD"


#define STRING_ONE              "1"
#define DEFAULT_ZERO_STRING     "00000000"

#define	CPU_LOG_SN		"CPULogSeqNum"
#define	MEM_LOG_SN		"MemLogSeqNum"

#define CHECKCLK_RESULT		"CheckClk"

#define	EARLY_INIT_CNT_A	"EarlyInitCntA"
#define	EARLY_INIT_CNT_B	"EarlyInitCntB"

#define	IP27LOG_NASID		"NASID"

#define	INV_CPU_MASK		"CpuMask"
#define	INV_MEM_MASK		"MemMask"
#define	INV_MEM_SZ		"MemorySize"
#define PREMIUM_BANKS		"Premium"

#define MEM_CFG_PREMIUM		"MemCfgPremium"
#define MEM_CFG_BANKSZ		"MemCfgBankSize"

#define	IP27LOG_DOMAIN		"DomainID"
#define	IP27LOG_CLUSTER		"ClusterID"
#define	IP27LOG_CELL		"CellID"
#define BACK_PLANE		"BackPlane"

#define	IP27LOG_DISABLE_IO	"DisableIO"
#define	IP27LOG_OVNIC		"OverrideNIC"
#define	IP27LOG_NASIDOFFSET	"NASIDOffset"
#define	IP27LOG_GMASTER		"GlobalMaster"
#define IP27LOG_FORCE_COARSE	"force_coarse"

#define IP27LOG_XBOX_NASID	"XboxNasid"
#define IP27LOG_ROUTE_VERBOSE		"Verbose"

#endif /* _ASM_SN_SN1_IP27LOG_H */
