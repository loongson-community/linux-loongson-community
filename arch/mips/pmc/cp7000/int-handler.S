#include <linux/config.h>
#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>

/*
 * galileo_handle_int -
 * We check for the timer first, then check PCI ints A and D.
 * Then check for serial IRQ and fall through.
 */
		.align	5
		NESTED(galileo_handle_int, PT_SIZE, sp)
		SAVE_ALL
		CLI
		.set	at
		mfc0	t0, CP0_CAUSE  
		mfc0	t2, CP0_STATUS

		and	t0, t2
        
#if 0
		andi	t1, t0, STATUSF_IP4	/* int2 hardware line (timer) */
		.set	noreorder
		bnez	t1, ll_galileo_irq
		 andi	t1, t0, STATUSF_IP2	/* int0 hardware line */
		bnez	t1, ll_pci_intA
		 andi	t1, t0, STATUSF_IP3	/* int1 hardware line */
		bnez	t1, ll_pci_intB
		 andi	t1, t0, STATUSF_IP4	/* int2 hardware line */
		bnez	t1, ll_pci_intC
		 andi	t1, t0, STATUSF_IP5	/* int3 hardware line */
		bnez	t1, ll_pci_intD
#endif
		 andi	t1, t0, STATUSF_IP6	/* int4 hardware line */
		bnez	t1, ll_galileo_irq
#if 0
		 andi	t1, t0, STATUSF_IP7	/* count / compare int */
		bnez	t1, ll_compare_irq
#endif
		.set	reorder

		/* wrong alarm or masked ... */
		j	spurious_interrupt
		nop
		END(galileo_handle_int)

		.align	5
ll_galileo_irq:
		li	a0, 4
		move	a1, sp
		jal	do_IRQ
		j	ret_from_irq

		.align	5
ll_compare_irq:
	PRINT("ll_compare_irq\n")
		li 	a0, 7
		move	a1, sp
		jal	do_IRQ
		j	ret_from_irq

		.align	5
ll_pci_intA:
	PRINT("ll_pci_intA\n")
		move	a0, sp
		jal	pci_intA
		j	ret_from_irq

		.align	5
ll_pci_intB:	
	PRINT("ll_pci_intB\n")
		move 	a0, sp
		jal	pci_intB
		j	ret_from_irq

		.align	5
ll_pci_intC:
	PRINT("ll_pci_intC\n")
		move 	a0, sp
		jal	pci_intC
		j	ret_from_irq

		.align	5
ll_pci_intD:
	PRINT("ll_pci_intD\n")
		move 	a0,sp
		jal	pci_intD
		j	ret_from_irq

		.align	5
ll_serial_irq:	
	PRINT("ll_serial_irq\n")
		li	a0,6
		move	a1,sp
		jal	do_IRQ
		j	ret_from_irq
