#include <linux/config.h>

#ifdef CONFIG_BOOT_ELF32
#include "arch/mips64/ld.script.elf32.S"
#endif

/*
 * The 64-bit ELF tools are pretty broken so at this time we generate 64-bit
 * ELF files from 32-bit files by conversion.
 */
#ifdef CONFIG_BOOT_ELF64
#include "arch/mips64/ld.script.elf64.S"
#endif
