/*
 * linux/arch/mips/mm/extable.c
 */
#include <linux/module.h>
#include <linux/spinlock.h>
#include <asm/branch.h>
#include <asm/uaccess.h>

/* Simple binary search */
const struct exception_table_entry *
search_extable(const struct exception_table_entry *first,
	       const struct exception_table_entry *last,
	       unsigned long value)
{
        while (first <= last) {
		const struct exception_table_entry *mid;
		long diff;

		mid = (last - first) / 2 + first;
		diff = mid->insn - value;
		if (diff == 0)
			return mid;
		else if (diff < 0)
			first = mid+1;
		else
			last = mid-1;
	}
	return NULL;
}

int fixup_exception(struct pt_regs *regs)
{
	const struct exception_table_entry *fixup;

	fixup = search_exception_tables(exception_epc(regs));
	if (fixup) {
		regs->cp0_epc = fixup->nextinsn;

		return 1;
	}

	return 0;
}
