#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/suspend.h>
#include <linux/module.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/reboot.h>
#include <linux/delay.h>

#ifndef CONFIG_HIBERNATION
unsigned long saved_context_s0, saved_context_s1, saved_context_s2;
unsigned long saved_context_s3, saved_context_s4, saved_context_s5;
unsigned long saved_context_s6, saved_context_s7, saved_context_s8;
unsigned long saved_context_gp, saved_context_sp;
unsigned long saved_context_ra;
#endif

unsigned long saved_context_k0, saved_context_k1, saved_context_jp;

unsigned long saved_context_status, saved_context_context;
unsigned long saved_context_index, saved_context_entrylo0, saved_context_entrylo1;
unsigned long saved_context_pagemask, saved_context_wired, saved_context_diag;
unsigned long saved_context_entryhi, saved_context_config, saved_context_taglo, saved_context_taghi;

extern asmlinkage void ls2f_save_regs(void);
extern asmlinkage void ls2f_suspend_ret(void);
extern asmlinkage void ls2f_board_standby(void);

void mach_suspend(void)
{
	int64_t *ret_ptr = (int64_t *)CKSEG1ADDR(0x100);
	int64_t *ptr = (int64_t *)CKSEG1ADDR(0x110);
	unsigned long long mem1 = *(unsigned volatile long long *) 0x900000003ff00010;
	unsigned long long mem2 = *(unsigned volatile long long *) 0x900000003ff00030;
	unsigned long long mem3 = *(unsigned volatile long long *) 0x900000003ff00050;

	*ret_ptr = (int64_t)CKSEG0ADDR((int64_t)&ls2f_suspend_ret);
	*ptr = (int64_t)0x5a5a5a5aa5a5a5a5;
	ls2f_save_regs();

	*ptr = (int64_t)0xaddeaddeaddeadde;
	*(unsigned volatile long long *) 0x900000003ff00010 = mem1;
	*(unsigned volatile long long *) 0x900000003ff00030 = mem2;
	*(unsigned volatile long long *) 0x900000003ff00050 = mem3;

	__asm__ __volatile__ (       "lui    $12,0xa000;\n"
			"ori    $12,$12,0x110;\n"
			"lui    $13,0xbcbc;\n"
			"ori    $13,$13,0xbcbc;\n"
			"sw     $13,0($12);\n"
			"sw     $13,4($12);\n"
			"ld	$13,56($29);\n"
			"sd	$13,8($12);\n"
			"lui	$12,0xbff0;\n"
			"ori	$12,$12,0x3f8;\n"
			"sb     $13,($12);\n"
			"sll	$13,$13,8;\n"
			"sb     $13,($12);\n"
			"sll	$13,$13,8;\n"
			"sb     $13,($12);\n"
			"sll	$13,$13,8;\n"
			"sb     $13,($12);\n"
			:::"$12", "$13");
}
