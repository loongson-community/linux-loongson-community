/*
 *  arch/mips/mm/fault.c
 *
 *  Copyright (C) 1991, 1992, 1993, 1994  Linus Torvalds
 *  Ported to MIPS by Ralf Baechle
 */
#include <linux/config.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/head.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include <linux/mm.h>

#include <asm/system.h>
#include <asm/segment.h>
#include <asm/pgtable.h>

extern void die_if_kernel(char *, struct pt_regs *, long);

/*
 * This routine handles page faults.  It determines the address,
 * and the problem, and then passes it off to one of the appropriate
 * routines.
 */
asmlinkage void
do_page_fault(struct pt_regs *regs, unsigned long writeaccess)
{
	struct vm_area_struct * vma;
	unsigned long address;

	/* get the address */
	__asm__(".set\tmips3\n\t"
                "dmfc0\t%0,$8\n\t"
                ".set\tmips0"
	        : "=r" (address));

#if 0
	printk("do_page_fault() #1: %s %08lx (epc == %08lx)\n",
	       writeaccess ? "writeaccess to" : "readaccess from",
	       address, regs->cp0_epc);
#endif
	vma = find_vma(current, address);
	if (!vma)
		goto bad_area;
	if (vma->vm_start <= address)
		goto good_area;
	if (!(vma->vm_flags & VM_GROWSDOWN))
		goto bad_area;
	if (vma->vm_end - address > current->rlim[RLIMIT_STACK].rlim_cur)
		goto bad_area;
	vma->vm_offset -= vma->vm_start - (address & PAGE_MASK);
	vma->vm_start = (address & PAGE_MASK);
/*
 * Ok, we have a good vm_area for this memory access, so
 * we can handle it..
 */
good_area:
	if (writeaccess) {
		if (!(vma->vm_flags & VM_WRITE))
			goto bad_area;
	} else {
		if (!(vma->vm_flags & (VM_READ | VM_EXEC)))
			goto bad_area;
	}
	handle_mm_fault(vma, address, writeaccess);
        return;

/*
 * Something tried to access memory that isn't in our memory map..
 * Fix it, but check if it's kernel or user first..
 */
bad_area:
	if (user_mode(regs)) {
		current->tss.cp0_badvaddr = address;
		current->tss.error_code = writeaccess;
		send_sig(SIGSEGV, current, 1);
		return;
	}
	/*
	 * Oops. The kernel tried to access some bad page. We'll have to
	 * terminate things with extreme prejudice.
	 */
	printk(KERN_ALERT "Unable to handle kernel paging request at virtual address %08lx",
	       address);
	die_if_kernel("Oops", regs, writeaccess);
	do_exit(SIGKILL);
}
