#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <asm/bugs.h>
#include <asm/cpu.h>
#include <asm/fpu.h>
#include <asm/mipsregs.h>

/*
 * Not all of the MIPS CPUs have the "wait" instruction available. Moreover,
 * the implementation of the "wait" feature differs between CPU families. This
 * points to the function that implements CPU specific wait.
 * The wait instruction stops the pipeline and reduces the power consumption of
 * the CPU very much.
 */
void (*cpu_wait)(void) = NULL;

static void r3081_wait(void)
{
	unsigned long cfg = read_c0_conf();
	write_c0_conf(cfg | R30XX_CONF_HALT);
}

static void r39xx_wait(void)
{
	unsigned long cfg = read_c0_conf();
	write_c0_conf(cfg | TX39_CONF_HALT);
}

static void r4k_wait(void)
{
	__asm__(".set\tmips3\n\t"
		"wait\n\t"
		".set\tmips0");
}

void au1k_wait(void)
{
#ifdef CONFIG_PM
	/* using the wait instruction makes CP0 counter unusable */
	__asm__(".set\tmips3\n\t"
		"wait\n\t"
		"nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" ".set\tmips0");
#else
	__asm__("nop\n\t" "nop\n\t");
#endif
}

static inline void check_wait(void)
{
	printk("Checking for 'wait' instruction... ");
	switch (current_cpu_data.cputype) {
	case CPU_R3081:
	case CPU_R3081E:
		cpu_wait = r3081_wait;
		printk(" available.\n");
		break;
	case CPU_TX3927:
		cpu_wait = r39xx_wait;
		printk(" available.\n");
		break;
	case CPU_R4200:
/*	case CPU_R4300: */
	case CPU_R4600:
	case CPU_R4640:
	case CPU_R4650:
	case CPU_R4700:
	case CPU_R5000:
	case CPU_NEVADA:
	case CPU_RM7000:
	case CPU_TX49XX:
	case CPU_4KC:
	case CPU_4KEC:
	case CPU_4KSC:
	case CPU_5KC:
/*	case CPU_20KC:*/
		cpu_wait = r4k_wait;
		printk(" available.\n");
		break;
	case CPU_AU1000:
	case CPU_AU1100:
	case CPU_AU1500:
		cpu_wait = au1k_wait;
		printk(" available.\n");
		break;
	default:
		printk(" unavailable.\n");
		break;
	}
}

void __init check_bugs(void)
{
	check_wait();
}

/*
 * Probe whether cpu has config register by trying to play with
 * alternate cache bit and see whether it matters.
 * It's used by cpu_probe to distinguish between R3000A and R3081.
 */
static inline int cpu_has_confreg(void)
{
#ifdef CONFIG_CPU_R3000
	extern unsigned long r3k_cache_size(unsigned long);
	unsigned long size1, size2;
	unsigned long cfg = read_c0_conf();

	size1 = r3k_cache_size(ST0_ISC);
	write_c0_conf(cfg ^ R30XX_CONF_AC);
	size2 = r3k_cache_size(ST0_ISC);
	write_c0_conf(cfg);
	return size1 != size2;
#else
	return 0;
#endif
}

/*
 * Get the FPU Implementation/Revision.
 */
static inline unsigned long cpu_get_fpu_id(void)
{
	unsigned long tmp, fpu_id;

	tmp = read_c0_status();
	__enable_fpu();
	fpu_id = read_32bit_cp1_register(CP1_REVISION);
	write_c0_status(tmp);
	return fpu_id;
}

/*
 * Check the CPU has an FPU the official way.
 */
static inline int __cpu_has_fpu(void)
{
	return ((cpu_get_fpu_id() & 0xff00) != FPIR_IMP_NONE);
}

#define R4K_OPTS (MIPS_CPU_TLB | MIPS_CPU_4KEX | MIPS_CPU_4KTLB \
		| MIPS_CPU_COUNTER | MIPS_CPU_CACHE_CDEX)

__init void cpu_probe(void)
{
#if defined(CONFIG_CPU_MIPS32) || defined(CONFIG_CPU_MIPS64)
	unsigned long config0 = read_c0_config();
	unsigned long config1;
#endif

	current_cpu_data.processor_id	= PRID_IMP_UNKNOWN;
	current_cpu_data.fpu_id		= FPIR_IMP_NONE;
	current_cpu_data.cputype	= CPU_UNKNOWN;

#if defined(CONFIG_CPU_MIPS32) || defined(CONFIG_CPU_MIPS64)
        if (config0 & (1 << 31)) {
		/* MIPS32 or MIPS64 compliant CPU. Read Config 1 register. */
		current_cpu_data.options = MIPS_CPU_TLB | MIPS_CPU_4KEX |
			MIPS_CPU_4KTLB | MIPS_CPU_COUNTER | MIPS_CPU_DIVEC |
			MIPS_CPU_LLSC;
		config1 = read_c0_config1();
		if (config1 & (1 << 3))
			current_cpu_data.options |= MIPS_CPU_WATCH;
		if (config1 & (1 << 2))
			current_cpu_data.options |= MIPS_CPU_MIPS16;
		if (config1 & (1 << 1))
			current_cpu_data.options |= MIPS_CPU_EJTAG;
		if (config1 & 1) {
			current_cpu_data.options |= MIPS_CPU_FPU;
#if defined(CONFIG_CPU_MIPS64)
			current_cpu_data.options |= MIPS_CPU_32FPR;
#endif
		}
		current_cpu_data.scache.flags = MIPS_CACHE_NOT_PRESENT;
	}
#endif
	current_cpu_data.processor_id = read_c0_prid();
	switch (current_cpu_data.processor_id & 0xff0000) {
	case PRID_COMP_LEGACY:
		switch (current_cpu_data.processor_id & 0xff00) {
		case PRID_IMP_R2000:
			current_cpu_data.cputype = CPU_R2000;
			current_cpu_data.isa_level = MIPS_CPU_ISA_I;
			current_cpu_data.options = MIPS_CPU_TLB |
			                           MIPS_CPU_NOFPUEX |
			                           MIPS_CPU_LLSC;
			if (__cpu_has_fpu())
				current_cpu_data.options |= MIPS_CPU_FPU;
			current_cpu_data.tlbsize = 64;
			break;
		case PRID_IMP_R3000:
			if ((current_cpu_data.processor_id & 0xff) == PRID_REV_R3000A)
				if (cpu_has_confreg())
					current_cpu_data.cputype = CPU_R3081E;
				else
					current_cpu_data.cputype = CPU_R3000A;
			else
				current_cpu_data.cputype = CPU_R3000;
			current_cpu_data.isa_level = MIPS_CPU_ISA_I;
			current_cpu_data.options = MIPS_CPU_TLB |
			                           MIPS_CPU_NOFPUEX |
			                           MIPS_CPU_LLSC;
			if (__cpu_has_fpu())
				current_cpu_data.options |= MIPS_CPU_FPU;
			current_cpu_data.tlbsize = 64;
			break;
		case PRID_IMP_R4000:
			if ((current_cpu_data.processor_id & 0xff) >= PRID_REV_R4400)
				current_cpu_data.cputype = CPU_R4400SC;
			else
				current_cpu_data.cputype = CPU_R4000SC;
			current_cpu_data.isa_level = MIPS_CPU_ISA_III;
			current_cpu_data.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR | MIPS_CPU_WATCH |
			                   MIPS_CPU_VCE | MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 48;
			break;
                case PRID_IMP_VR41XX:
			switch (current_cpu_data.processor_id & 0xf0) {
#ifndef CONFIG_VR4181
			case PRID_REV_VR4111:
				current_cpu_data.cputype = CPU_VR4111;
				break;
#else
			case PRID_REV_VR4181:
				current_cpu_data.cputype = CPU_VR4181;
				break;
#endif
			case PRID_REV_VR4121:
				current_cpu_data.cputype = CPU_VR4121;
				break;
			case PRID_REV_VR4122:
				if ((current_cpu_data.processor_id & 0xf) < 0x3)
					current_cpu_data.cputype = CPU_VR4122;
				else
					current_cpu_data.cputype = CPU_VR4181A;
				break;
			case PRID_REV_VR4131:
				current_cpu_data.cputype = CPU_VR4131;
				break;
			default:
				printk(KERN_INFO "Unexpected CPU of NEC VR4100 series\n");
				current_cpu_data.cputype = CPU_VR41XX;
				break;
			}
                        current_cpu_data.isa_level = MIPS_CPU_ISA_III;
                        current_cpu_data.options = R4K_OPTS;
                        current_cpu_data.tlbsize = 32;
                        break;
		case PRID_IMP_R4300:
			current_cpu_data.cputype = CPU_R4300;
			current_cpu_data.isa_level = MIPS_CPU_ISA_III;
			current_cpu_data.options = R4K_OPTS | MIPS_CPU_FPU |
					   MIPS_CPU_32FPR | MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 32;
			break;
		case PRID_IMP_R4600:
			current_cpu_data.cputype = CPU_R4600;
			current_cpu_data.isa_level = MIPS_CPU_ISA_III;
			current_cpu_data.options = R4K_OPTS | MIPS_CPU_FPU |
			                           MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 48;
			break;
		#if 0
 		case PRID_IMP_R4650:
			/*
			 * This processor doesn't have an MMU, so it's not
			 * "real easy" to run Linux on it. It is left purely
			 * for documentation.  Commented out because it shares
			 * it's c0_prid id number with the TX3900.
			 */
	 		current_cpu_data.cputype = CPU_R4650;
		 	current_cpu_data.isa_level = MIPS_CPU_ISA_III;
			current_cpu_data.options = R4K_OPTS | MIPS_CPU_FPU |
			                           MIPS_CPU_LLSC;
		        current_cpu_data.tlbsize = 48;
			break;
		#endif
		case PRID_IMP_TX39:
			current_cpu_data.isa_level = MIPS_CPU_ISA_I;
			current_cpu_data.options = MIPS_CPU_TLB;

			if ((current_cpu_data.processor_id & 0xf0) ==
			    (PRID_REV_TX3927 & 0xf0)) {
				current_cpu_data.cputype = CPU_TX3927;
				current_cpu_data.tlbsize = 64;
			} else {
				switch (current_cpu_data.processor_id & 0xff) {
				case PRID_REV_TX3912:
					current_cpu_data.cputype = CPU_TX3912;
					current_cpu_data.tlbsize = 32;
					break;
				case PRID_REV_TX3922:
					current_cpu_data.cputype = CPU_TX3922;
					current_cpu_data.tlbsize = 64;
					break;
				default:
					current_cpu_data.cputype = CPU_UNKNOWN;
					break;
				}
			}
			break;
		case PRID_IMP_R4700:
			current_cpu_data.cputype = CPU_R4700;
			current_cpu_data.isa_level = MIPS_CPU_ISA_III;
			current_cpu_data.options = R4K_OPTS | MIPS_CPU_FPU |
			                           MIPS_CPU_32FPR |
			                           MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 48;
			break;
		case PRID_IMP_TX49:
			current_cpu_data.cputype = CPU_TX49XX;
			current_cpu_data.isa_level = MIPS_CPU_ISA_III;
			current_cpu_data.options = R4K_OPTS | MIPS_CPU_FPU |
			                           MIPS_CPU_32FPR |
			                           MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 48;
			break;
		case PRID_IMP_R5000:
			current_cpu_data.cputype = CPU_R5000;
			current_cpu_data.isa_level = MIPS_CPU_ISA_IV;
			current_cpu_data.options = R4K_OPTS | MIPS_CPU_FPU |
			                           MIPS_CPU_32FPR |
			                           MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 48;
			break;
		case PRID_IMP_R5432:
			current_cpu_data.cputype = CPU_R5432;
			current_cpu_data.isa_level = MIPS_CPU_ISA_IV;
			current_cpu_data.options = R4K_OPTS | MIPS_CPU_FPU |
			                           MIPS_CPU_32FPR |
			                           MIPS_CPU_WATCH |
			                           MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 48;
			break;
		case PRID_IMP_R5500:
			current_cpu_data.cputype = CPU_R5500;
			current_cpu_data.isa_level = MIPS_CPU_ISA_IV;
			current_cpu_data.options = R4K_OPTS | MIPS_CPU_FPU |
			                           MIPS_CPU_32FPR |
			                           MIPS_CPU_WATCH |
			                           MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 48;
			break;
		case PRID_IMP_NEVADA:
			current_cpu_data.cputype = CPU_NEVADA;
			current_cpu_data.isa_level = MIPS_CPU_ISA_IV;
			current_cpu_data.options = R4K_OPTS | MIPS_CPU_FPU |
			                           MIPS_CPU_32FPR |
			                           MIPS_CPU_DIVEC |
			                           MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 48;
			break;
		case PRID_IMP_R6000:
			current_cpu_data.cputype = CPU_R6000;
			current_cpu_data.isa_level = MIPS_CPU_ISA_II;
			current_cpu_data.options = MIPS_CPU_TLB |
			                           MIPS_CPU_FPU |
			                           MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 32;
			break;
		case PRID_IMP_R6000A:
			current_cpu_data.cputype = CPU_R6000A;
			current_cpu_data.isa_level = MIPS_CPU_ISA_II;
			current_cpu_data.options = MIPS_CPU_TLB |
			                           MIPS_CPU_FPU |
			                           MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 32;
			break;
		case PRID_IMP_RM7000:
			current_cpu_data.cputype = CPU_RM7000;
			current_cpu_data.isa_level = MIPS_CPU_ISA_IV;
			current_cpu_data.options = R4K_OPTS | MIPS_CPU_FPU |
			                           MIPS_CPU_32FPR |
			                           MIPS_CPU_LLSC;
			/*
			 * Undocumented RM7000:  Bit 29 in the info register of
			 * the RM7000 v2.0 indicates if the TLB has 48 or 64
			 * entries.
			 *
			 * 29      1 =>    64 entry JTLB
			 *         0 =>    48 entry JTLB
			 */
			current_cpu_data.tlbsize = (read_c0_info() & (1 << 29)) ? 64 : 48;
			break;
		case PRID_IMP_R8000:
			current_cpu_data.cputype = CPU_R8000;
			current_cpu_data.isa_level = MIPS_CPU_ISA_IV;
			current_cpu_data.options = MIPS_CPU_TLB |
			                           MIPS_CPU_4KEX |
				                   MIPS_CPU_FPU |
			                           MIPS_CPU_32FPR |
			                           MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 384;      /* has weird TLB: 3-way x 128 */
			break;
		case PRID_IMP_R10000:
			current_cpu_data.cputype = CPU_R10000;
			current_cpu_data.isa_level = MIPS_CPU_ISA_IV;
			current_cpu_data.options = MIPS_CPU_TLB |
			                           MIPS_CPU_4KEX |
				                   MIPS_CPU_FPU |
			                           MIPS_CPU_32FPR |
				                   MIPS_CPU_COUNTER |
			                           MIPS_CPU_WATCH |
			                           MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 64;
			break;
		case PRID_IMP_R12000:
			current_cpu_data.cputype = CPU_R12000;
			current_cpu_data.isa_level = MIPS_CPU_ISA_IV;
			current_cpu_data.options = MIPS_CPU_TLB |
			                           MIPS_CPU_4KEX |
				                   MIPS_CPU_FPU |
			                           MIPS_CPU_32FPR |
				                   MIPS_CPU_COUNTER |
			                           MIPS_CPU_WATCH |
			                           MIPS_CPU_LLSC;
			current_cpu_data.tlbsize = 64;
			break;
		default:
			current_cpu_data.cputype = CPU_UNKNOWN;
			break;
		}
		break;
#if defined(CONFIG_CPU_MIPS32) || defined(CONFIG_CPU_MIPS64)
	case PRID_COMP_MIPS:
		switch (current_cpu_data.processor_id & 0xff00) {
		case PRID_IMP_4KC:
			current_cpu_data.cputype = CPU_4KC;
			current_cpu_data.isa_level = MIPS_CPU_ISA_M32;
			break;
		case PRID_IMP_4KEC:
			current_cpu_data.cputype = CPU_4KEC;
			current_cpu_data.isa_level = MIPS_CPU_ISA_M32;
			break;
		case PRID_IMP_4KSC:
			current_cpu_data.cputype = CPU_4KSC;
			current_cpu_data.isa_level = MIPS_CPU_ISA_M32;
			break;
		case PRID_IMP_5KC:
			current_cpu_data.cputype = CPU_5KC;
			current_cpu_data.isa_level = MIPS_CPU_ISA_M64;
			break;
		case PRID_IMP_20KC:
			current_cpu_data.cputype = CPU_20KC;
			current_cpu_data.isa_level = MIPS_CPU_ISA_M64;
			break;
		default:
			current_cpu_data.cputype = CPU_UNKNOWN;
			break;
		}
		break;
	case PRID_COMP_ALCHEMY:
		switch (current_cpu_data.processor_id & 0xff00) {
		case PRID_IMP_AU1_REV1:
		case PRID_IMP_AU1_REV2:
			switch ((current_cpu_data.processor_id >> 24) & 0xff) {
			case 0:
 				current_cpu_data.cputype = CPU_AU1000;
				break;
			case 1:
				current_cpu_data.cputype = CPU_AU1500;
				break;
			case 2:
				current_cpu_data.cputype = CPU_AU1100;
				break;
			default:
				panic("Unknown Au Core!");
				break;
			}
			current_cpu_data.isa_level = MIPS_CPU_ISA_M32;
 			break;
		default:
			current_cpu_data.cputype = CPU_UNKNOWN;
			break;
		}
		break;
#endif /* CONFIG_CPU_MIPS32 */
	case PRID_COMP_SIBYTE:
		switch (current_cpu_data.processor_id & 0xff00) {
		case PRID_IMP_SB1:
			current_cpu_data.cputype = CPU_SB1;
			current_cpu_data.isa_level = MIPS_CPU_ISA_M64;
			current_cpu_data.options = MIPS_CPU_TLB |
			                           MIPS_CPU_4KEX |
			                           MIPS_CPU_COUNTER |
			                           MIPS_CPU_DIVEC |
			                           MIPS_CPU_MCHECK |
			                           MIPS_CPU_EJTAG |
			                           MIPS_CPU_WATCH |
			                           MIPS_CPU_LLSC;
#ifndef CONFIG_SB1_PASS_1_WORKAROUNDS
			/* FPU in pass1 is known to have issues. */
			current_cpu_data.options |= MIPS_CPU_FPU | MIPS_CPU_32FPR;
#endif
			break;
		default:
			current_cpu_data.cputype = CPU_UNKNOWN;
			break;
		}
		break;
	default:
		current_cpu_data.cputype = CPU_UNKNOWN;
	}
	if (current_cpu_data.options & MIPS_CPU_FPU)
		current_cpu_data.fpu_id = cpu_get_fpu_id();
}

__init void cpu_report(void)
{
	printk("CPU revision is: %08x\n", current_cpu_data.processor_id);
	if (current_cpu_data.options & MIPS_CPU_FPU)
		printk("FPU revision is: %08x\n", current_cpu_data.fpu_id);
}
