#include <linux/config.h>
#include <linux/smp.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

static struct proc_dir_entry * root_irq_dir;
static struct proc_dir_entry * irq_dir [NR_IRQS];

#define HEX_DIGITS 8

static unsigned int parse_hex_value (const char *buffer,
		unsigned long count, unsigned long *ret)
{
	unsigned char hexnum [HEX_DIGITS];
	unsigned long value;
	int i;

	if (!count)
		return -EINVAL;
	if (count > HEX_DIGITS)
		count = HEX_DIGITS;
	if (copy_from_user(hexnum, buffer, count))
		return -EFAULT;

	/*
	 * Parse the first 8 characters as a hex string, any non-hex char
	 * is end-of-string. '00e1', 'e1', '00E1', 'E1' are all the same.
	 */
	value = 0;

	for (i = 0; i < count; i++) {
		unsigned int c = hexnum[i];

		switch (c) {
			case '0' ... '9': c -= '0'; break;
			case 'a' ... 'f': c -= 'a'-10; break;
			case 'A' ... 'F': c -= 'A'-10; break;
		default:
			goto out;
		}
		value = (value << 4) | c;
	}
out:
	*ret = value;
	return 0;
}

#if CONFIG_SMP

static struct proc_dir_entry * smp_affinity_entry [NR_IRQS];

static unsigned long irq_affinity [NR_IRQS] = { [0 ... NR_IRQS-1] = ~0UL };
static int irq_affinity_read_proc (char *page, char **start, off_t off,
			int count, int *eof, void *data)
{
	if (count < HEX_DIGITS+1)
		return -EINVAL;
	return sprintf (page, "%08lx\n", irq_affinity[(long)data]);
}

static int irq_affinity_write_proc (struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	int irq = (long) data, full_count = count, err;
	unsigned long new_value;

	if (!irq_desc[irq].handler->set_affinity)
		return -EIO;

	err = parse_hex_value(buffer, count, &new_value);

	/*
	 * Do not allow disabling IRQs completely - it's a too easy
	 * way to make the system unusable accidentally :-) At least
	 * one online CPU still has to be targeted.
	 */
	if (!(new_value & cpu_online_map))
		return -EINVAL;

	irq_affinity[irq] = new_value;
	irq_desc[irq].handler->set_affinity(irq, new_value);

	return full_count;
}

#endif

static int prof_cpu_mask_read_proc (char *page, char **start, off_t off,
			int count, int *eof, void *data)
{
	unsigned long *mask = (unsigned long *) data;
	if (count < HEX_DIGITS+1)
		return -EINVAL;
	return sprintf (page, "%08lx\n", *mask);
}

static int prof_cpu_mask_write_proc (struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	unsigned long *mask = (unsigned long *) data, full_count = count, err;
	unsigned long new_value;

	err = parse_hex_value(buffer, count, &new_value);
	if (err)
		return err;

	*mask = new_value;
	return full_count;
}

#define MAX_NAMELEN 10

static void register_irq_proc (unsigned int irq)
{
	char name [MAX_NAMELEN];

	if (!root_irq_dir || (irq_desc[irq].handler == &no_irq_type) ||
			irq_dir[irq])
		return;

	memset(name, 0, MAX_NAMELEN);
	sprintf(name, "%d", irq);

	/* create /proc/irq/1234 */
	irq_dir[irq] = proc_mkdir(name, root_irq_dir);

#if CONFIG_SMP
	{
		struct proc_dir_entry *entry;

		/* create /proc/irq/1234/smp_affinity */
		entry = create_proc_entry("smp_affinity", 0600, irq_dir[irq]);

		if (entry) {
			entry->nlink = 1;
			entry->data = (void *)(long)irq;
			entry->read_proc = irq_affinity_read_proc;
			entry->write_proc = irq_affinity_write_proc;
		}

		smp_affinity_entry[irq] = entry;
	}
#endif
}

unsigned long prof_cpu_mask = -1;

void init_irq_proc (void)
{
	struct proc_dir_entry *entry;
	int i;

	/* create /proc/irq */
	root_irq_dir = proc_mkdir("irq", 0);

	/* create /proc/irq/prof_cpu_mask */
	entry = create_proc_entry("prof_cpu_mask", 0600, root_irq_dir);

	if (!entry)
	    return;

	entry->nlink = 1;
	entry->data = (void *)&prof_cpu_mask;
	entry->read_proc = prof_cpu_mask_read_proc;
	entry->write_proc = prof_cpu_mask_write_proc;

	/*
	 * Create entries for all existing IRQs.
	 */
	for (i = 0; i < NR_IRQS; i++)
		register_irq_proc(i);
}
