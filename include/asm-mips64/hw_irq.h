/*
 * This exists merely to satisfy <linux/irq.h>.  There is
 * nothing that would go here of general interest.
 *
 * Everything of consequence is in arch/alpha/kernel/irq_impl.h,
 * to be used only in arch/alpha/kernel/.
 */

/*
 * Special interrupt numbers for SMP
 *
 * XXX ATM this is IP27 specific.
 */
#define DORESCHED       0xab
#define DOCALL          0xbc
