/*
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2000 - 2001 by Kanoj Sarcar (kanoj@sgi.com)
 * Copyright (C) 2000 - 2001 by Silicon Graphics, Inc.
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/mmzone.h>	/* for numnodes */
#include <linux/mm.h>
#include <linux/cpumask.h>
#include <asm/cpu.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>
#include <asm/sn/types.h>
#include <asm/sn/sn0/addrs.h>
#include <asm/sn/sn0/hubni.h>
#include <asm/sn/sn0/hubio.h>
#include <asm/sn/klconfig.h>
#include <asm/sn/ioc3.h>
#include <asm/mipsregs.h>
#include <asm/sn/gda.h>
#include <asm/sn/intr.h>
#include <asm/current.h>
#include <asm/smp.h>
#include <asm/processor.h>
#include <asm/mmu_context.h>
#include <asm/thread_info.h>
#include <asm/sn/launch.h>
#include <asm/sn/sn_private.h>
#include <asm/sn/sn0/ip27.h>
#include <asm/sn/mapped_kernel.h>

#define CPU_NONE		(cpuid_t)-1

/*
 * The following should work till 64 nodes, ie 128p SN0s.
 */
#define CNODEMASK_CLRALL(p)	(p) = 0
#define CNODEMASK_TSTB(p, bit)	((p) & (1ULL << (bit)))
#define CNODEMASK_SETB(p, bit)	((p) |= 1ULL << (bit))

static hubreg_t region_mask;
static int	fine_mode;
static int maxcpus;
static spinlock_t hub_mask_lock = SPIN_LOCK_UNLOCKED;
static cnodemask_t hub_init_mask;
static atomic_t numstarted = ATOMIC_INIT(1);
static int router_distance;
nasid_t master_nasid = INVALID_NASID;

cnodeid_t	nasid_to_compact_node[MAX_NASIDS];
nasid_t		compact_to_nasid_node[MAX_COMPACT_NODES];
cnodeid_t	cpuid_to_compact_node[MAXCPUS];
char		node_distances[MAX_COMPACT_NODES][MAX_COMPACT_NODES];

static hubreg_t get_region(cnodeid_t cnode)
{
	if (fine_mode)
		return COMPACT_TO_NASID_NODEID(cnode) >> NASID_TO_FINEREG_SHFT;
	else
		return COMPACT_TO_NASID_NODEID(cnode) >> NASID_TO_COARSEREG_SHFT;
}

static void gen_region_mask(hubreg_t *region_mask, int maxnodes)
{
	cnodeid_t cnode;

	(*region_mask) = 0;
	for (cnode = 0; cnode < maxnodes; cnode++) {
		(*region_mask) |= 1ULL << get_region(cnode);
	}
}

static int is_fine_dirmode(void)
{
	return (((LOCAL_HUB_L(NI_STATUS_REV_ID) & NSRI_REGIONSIZE_MASK)
		>> NSRI_REGIONSIZE_SHFT) & REGIONSIZE_FINE);
}

static nasid_t get_actual_nasid(lboard_t *brd)
{
	klhub_t *hub;

	if (!brd)
		return INVALID_NASID;

	/* find out if we are a completely disabled brd. */
	hub  = (klhub_t *)find_first_component(brd, KLSTRUCT_HUB);
	if (!hub)
		return INVALID_NASID;
	if (!(hub->hub_info.flags & KLINFO_ENABLE))	/* disabled node brd */
		return hub->hub_info.physid;
	else
		return brd->brd_nasid;
}

static int do_cpumask(cnodeid_t cnode, nasid_t nasid, int highest)
{
	static int tot_cpus_found = 0;
	lboard_t *brd;
	klcpu_t *acpu;
	int cpus_found = 0;
	cpuid_t cpuid;

	brd = find_lboard((lboard_t *)KL_CONFIG_INFO(nasid), KLTYPE_IP27);

	do {
		acpu = (klcpu_t *)find_first_component(brd, KLSTRUCT_CPU);
		while (acpu) {
			cpuid = acpu->cpu_info.virtid;
			/* cnode is not valid for completely disabled brds */
			if (get_actual_nasid(brd) == brd->brd_nasid)
				cpuid_to_compact_node[cpuid] = cnode;
			if (cpuid > highest)
				highest = cpuid;
			/* Only let it join in if it's marked enabled */
			if ((acpu->cpu_info.flags & KLINFO_ENABLE) &&
			    (tot_cpus_found != NR_CPUS)) {
				cpu_set(cpuid, cpus_found);
				cpus_found++;
				tot_cpus_found++;
			}
			acpu = (klcpu_t *)find_component(brd, (klinfo_t *)acpu,
								KLSTRUCT_CPU);
		}
		brd = KLCF_NEXT(brd);
		if (!brd)
			break;

		brd = find_lboard(brd, KLTYPE_IP27);
	} while (brd);

	return cpus_found;
}

static cpuid_t cpu_node_probe(void)
{
	int i, highest = 0;
	gda_t *gdap = GDA;

	/*
	 * Initialize the arrays to invalid nodeid (-1)
	 */
	for (i = 0; i < MAX_COMPACT_NODES; i++)
		compact_to_nasid_node[i] = INVALID_NASID;
	for (i = 0; i < MAX_NASIDS; i++)
		nasid_to_compact_node[i] = INVALID_CNODEID;
	for (i = 0; i < MAXCPUS; i++)
		cpuid_to_compact_node[i] = INVALID_CNODEID;

	numnodes = 0;
	for (i = 0; i < MAX_COMPACT_NODES; i++) {
		nasid_t nasid = gdap->g_nasidtable[i];
		if (nasid == INVALID_NASID)
			break;
		compact_to_nasid_node[i] = nasid;
		nasid_to_compact_node[nasid] = i;
		numnodes++;
		highest = do_cpumask(i, nasid, highest);
	}

	/*
	 * Cpus are numbered in order of cnodes. Currently, disabled
	 * cpus are not numbered.
	 */

	return highest + 1;
}

void mlreset(void)
{
	int i;
	void init_topology_matrix(void);
	void dump_topology(void);


	master_nasid = get_nasid();
	fine_mode = is_fine_dirmode();

	/*
	 * Probe for all CPUs - this creates the cpumask and
	 * sets up the mapping tables.
	 */
	maxcpus = cpu_node_probe();
	printk("Discovered %d cpus on %d nodes\n", maxcpus, numnodes);

	init_topology_matrix();
	dump_topology();

	gen_region_mask(&region_mask, numnodes);
	CNODEMASK_CLRALL(hub_init_mask);

	setup_replication_mask(numnodes);

	/*
	 * Set all nodes' calias sizes to 8k
	 */
	for (i = 0; i < numnodes; i++) {
		nasid_t nasid;

		nasid = COMPACT_TO_NASID_NODEID(i);

		/*
		 * Always have node 0 in the region mask, otherwise
		 * CALIAS accesses get exceptions since the hub
		 * thinks it is a node 0 address.
		 */
		REMOTE_HUB_S(nasid, PI_REGION_PRESENT, (region_mask | 1));
#ifdef CONFIG_REPLICATE_EXHANDLERS
		REMOTE_HUB_S(nasid, PI_CALIAS_SIZE, PI_CALIAS_SIZE_8K);
#else
		REMOTE_HUB_S(nasid, PI_CALIAS_SIZE, PI_CALIAS_SIZE_0);
#endif

#ifdef LATER
		/*
		 * Set up all hubs to have a big window pointing at
		 * widget 0. Memory mode, widget 0, offset 0
		 */
		REMOTE_HUB_S(nasid, IIO_ITTE(SWIN0_BIGWIN),
			((HUB_PIO_MAP_TO_MEM << IIO_ITTE_IOSP_SHIFT) |
			(0 << IIO_ITTE_WIDGET_SHIFT)));
#endif
	}
}

static void intr_clear_bits(nasid_t nasid, volatile hubreg_t *pend,
	int base_level, char *name)
{
	volatile hubreg_t bits;
	int i;

	/* Check pending interrupts */
	if ((bits = HUB_L(pend)) != 0)
		for (i = 0; i < N_INTPEND_BITS; i++)
			if (bits & (1 << i))
				LOCAL_HUB_CLR_INTR(base_level + i);
}

static void intr_clear_all(nasid_t nasid)
{
	REMOTE_HUB_S(nasid, PI_INT_MASK0_A, 0);
	REMOTE_HUB_S(nasid, PI_INT_MASK0_B, 0);
	REMOTE_HUB_S(nasid, PI_INT_MASK1_A, 0);
	REMOTE_HUB_S(nasid, PI_INT_MASK1_B, 0);
	intr_clear_bits(nasid, REMOTE_HUB_ADDR(nasid, PI_INT_PEND0),
		INT_PEND0_BASELVL, "INT_PEND0");
	intr_clear_bits(nasid, REMOTE_HUB_ADDR(nasid, PI_INT_PEND1),
		INT_PEND1_BASELVL, "INT_PEND1");
}

static void sn_mp_setup(void)
{
	cnodeid_t	cnode;
//	cpuid_t		cpu;

	for (cnode = 0; cnode < numnodes; cnode++) {
//		init_platform_nodepda();
		intr_clear_all(COMPACT_TO_NASID_NODEID(cnode));
	}
//	for (cpu = 0; cpu < maxcpus; cpu++)
//		init_platform_pda();
}

static void per_hub_init(cnodeid_t cnode)
{
	extern void pcibr_setup(cnodeid_t);
	cnodemask_t	done;
	nasid_t		nasid;

	nasid = COMPACT_TO_NASID_NODEID(cnode);

	spin_lock(&hub_mask_lock);
	/* Test our bit. */
	if (!(done = CNODEMASK_TSTB(hub_init_mask, cnode))) {
		/* Turn our bit on in the mask. */
		CNODEMASK_SETB(hub_init_mask, cnode);
		/*
	 	 * Do the actual initialization if it hasn't been done yet.
	 	 * We don't need to hold a lock for this work.
	 	 */
		/*
		 * Set CRB timeout at 5ms, (< PI timeout of 10ms)
		 */
		REMOTE_HUB_S(nasid, IIO_ICTP, 0x800);
		REMOTE_HUB_S(nasid, IIO_ICTO, 0xff);
		hub_rtc_init(cnode);
		pcibr_setup(cnode);
#ifdef CONFIG_REPLICATE_EXHANDLERS
		/*
		 * If this is not a headless node initialization,
		 * copy over the caliased exception handlers.
		 */
		if (get_compact_nodeid() == cnode) {
			extern char except_vec0, except_vec1_r10k;
			extern char except_vec2_generic, except_vec3_generic;

			memcpy((void *)(KSEG0 + 0x100), &except_vec2_generic,
								0x80);
			memcpy((void *)(KSEG0 + 0x180), &except_vec3_generic,
								0x80);
			memcpy((void *)KSEG0, &except_vec0, 0x80);
			memcpy((void *)KSEG0 + 0x080, &except_vec1_r10k, 0x80);
			memcpy((void *)(KSEG0 + 0x100), (void *) KSEG0, 0x80);
			memcpy((void *)(KSEG0 + 0x180), &except_vec3_generic,
								0x100);
			__flush_cache_all();
		}
#endif
	}
	spin_unlock(&hub_mask_lock);
}

void prom_init_secondary(void)
{
	extern void install_cpu_nmi_handler(int slice);
	static int is_slave = 0;
	int cpu = smp_processor_id();
	cnodeid_t cnode = get_compact_nodeid();

#if 0
	intr_init();
#endif
	clear_c0_status(ST0_IM);
	per_hub_init(cnode);
	cpu_time_init();
	if (smp_processor_id())	/* master can't do this early, no kmalloc */
		install_cpuintr(cpu);
	/* Install our NMI handler if symmon hasn't installed one. */
	install_cpu_nmi_handler(cputoslice(cpu));
#if 0
	install_tlbintr(cpu);
#endif
	set_c0_status(SRB_DEV0 | SRB_DEV1);
	if (is_slave) {
		clear_c0_status(ST0_BEV);
		if (current_cpu_data.isa_level == MIPS_CPU_ISA_IV)
			set_c0_status(ST0_XX);
		set_c0_status(ST0_KX|ST0_SX|ST0_UX);
		local_irq_enable();
		atomic_inc(&numstarted);
	} else {
		is_slave = 1;
	}
}

void prom_smp_finish(void)
{
}

cnodeid_t get_compact_nodeid(void)
{
	nasid_t nasid;

	nasid = get_nasid();
	/*
	 * Map the physical node id to a virtual node id (virtual node ids
	 * are contiguous).
	 */
	return NASID_TO_COMPACT_NODEID(nasid);
}

#ifdef CONFIG_SMP

/*
 * Takes as first input the PROM assigned cpu id, and the kernel
 * assigned cpu id as the second.
 */
static void alloc_cpupda(cpuid_t cpu, int cpunum)
{
	cnodeid_t	node;
	nasid_t		nasid;

	node = get_cpu_cnode(cpu);
	nasid = COMPACT_TO_NASID_NODEID(node);

	cputonasid(cpunum) = nasid;
	cputocnode(cpunum) = node;
	cputoslice(cpunum) = get_cpu_slice(cpu);
	cpu_data[cpunum].p_cpuid = cpu;
}

void __init prom_build_cpu_map(void)
{
	/* Work was already done in mlreset */
}

void __init prom_prepare_cpus(unsigned int max_cpus)
{
	sn_mp_setup();
	/* Master has already done per_cpu_init() */
	install_cpuintr(smp_processor_id());
#if 0
	bte_lateinit();
	ecc_init();
#endif

	replicate_kernel_text(numnodes);

	/* Done on master only */
	/* XXXKW need first == physical, second == logical */
	alloc_cpupda(cpu, 0);
}

/*
 * Launch a slave into smp_bootstrap().  It doesn't take an argument, and we
 * set sp to the kernel stack of the newly created idle process, gp to the proc
 * struct so that current_thread_info() will work.
 */
void __init prom_boot_secondary(int cpu, struct task_struct *idle)
{
	unsigned long gp = (unsigned long) idle->thread_info;
	unsigned long sp = gp + THREAD_SIZE - 32;

	LAUNCH_SLAVE(cputonasid(cpu),cputoslice(cpu),
		(launch_proc_t)MAPPED_KERN_RW_TO_K0(smp_bootstrap),
		0, (void *) sp, (void *) gp);
}

/* XXXKW implement prom_init_secondary() and prom_smp_finish to share
 * start_secondary with kernel/smp.c; otherwise, roll your own. */

void __init prom_cpus_done(void)
{
	cnodeid_t cnode;

#ifdef LATER
	Wait logic goes here.
#endif
	for (cnode = 0; cnode < numnodes; cnode++) {
#if 0
		if (cnodetocpu(cnode) == -1) {
			printk("Initializing headless hub,cnode %d", cnode);
			per_hub_init(cnode);
		}
#endif
	}
#if 0
	cpu_io_setup();
	init_mfhi_war();
#endif
}

#endif /* CONFIG_SMP */


#define	rou_rflag	rou_flags

static void router_recurse(klrou_t *router_a, klrou_t *router_b, int depth)
{
	klrou_t *router;
	lboard_t *brd;
	int	port;

	if (router_a->rou_rflag == 1)
		return;

	if (depth >= router_distance)
		return;

	router_a->rou_rflag = 1;

	for (port = 1; port <= MAX_ROUTER_PORTS; port++) {
		if (router_a->rou_port[port].port_nasid == INVALID_NASID)
			continue;

		brd = (lboard_t *)NODE_OFFSET_TO_K0(
			router_a->rou_port[port].port_nasid,
			router_a->rou_port[port].port_offset);

		if (brd->brd_type == KLTYPE_ROUTER) {
			router = (klrou_t *)NODE_OFFSET_TO_K0(NASID_GET(brd), brd->brd_compts[0]);
			if (router == router_b) {
				if (depth < router_distance)
					router_distance = depth;
			}
			else
				router_recurse(router, router_b, depth + 1);
		}
	}

	router_a->rou_rflag = 0;
}

static int node_distance(nasid_t nasid_a, nasid_t nasid_b)
{
	klrou_t *router, *router_a = NULL, *router_b = NULL;
	lboard_t *brd, *dest_brd;
	cnodeid_t cnode;
	nasid_t nasid;
	int port;

	/* Figure out which routers nodes in question are connected to */
	for (cnode = 0; cnode < numnodes; cnode++) {
		nasid = COMPACT_TO_NASID_NODEID(cnode);

		if (nasid == -1) continue;

		brd = find_lboard_class((lboard_t *)KL_CONFIG_INFO(nasid),
					KLTYPE_ROUTER);

		if (!brd)
			continue;

		do {
			if (brd->brd_flags & DUPLICATE_BOARD)
				continue;

			router = (klrou_t *)NODE_OFFSET_TO_K0(NASID_GET(brd), brd->brd_compts[0]);
			router->rou_rflag = 0;

			for (port = 1; port <= MAX_ROUTER_PORTS; port++) {
				if (router->rou_port[port].port_nasid == INVALID_NASID)
					continue;

				dest_brd = (lboard_t *)NODE_OFFSET_TO_K0(
					router->rou_port[port].port_nasid,
					router->rou_port[port].port_offset);

				if (dest_brd->brd_type == KLTYPE_IP27) {
					if (dest_brd->brd_nasid == nasid_a)
						router_a = router;
					if (dest_brd->brd_nasid == nasid_b)
						router_b = router;
				}
			}

		} while ((brd = find_lboard_class(KLCF_NEXT(brd), KLTYPE_ROUTER)));
	}

	if (router_a == NULL) {
		printk("node_distance: router_a NULL\n");
		return -1;
	}
	if (router_b == NULL) {
		printk("node_distance: router_b NULL\n");
		return -1;
	}

	if (nasid_a == nasid_b)
		return 0;

	if (router_a == router_b)
		return 1;

	router_distance = 100;
	router_recurse(router_a, router_b, 2);

	return router_distance;
}

static void init_topology_matrix(void)
{
	nasid_t nasid, nasid2;
	cnodeid_t row, col;

	for (row = 0; row < MAX_COMPACT_NODES; row++)
		for (col = 0; col < MAX_COMPACT_NODES; col++)
			node_distances[row][col] = -1;

	for (row = 0; row < numnodes; row++) {
		nasid = COMPACT_TO_NASID_NODEID(row);
		for (col = 0; col < numnodes; col++) {
			nasid2 = COMPACT_TO_NASID_NODEID(col);
			node_distances[row][col] = node_distance(nasid, nasid2);
		}
	}
}

static void dump_topology(void)
{
	nasid_t nasid;
	cnodeid_t cnode;
	lboard_t *brd, *dest_brd;
	int port;
	int router_num = 0;
	klrou_t *router;
	cnodeid_t row, col;

	printk("************** Topology ********************\n");

	printk("    ");
	for (col = 0; col < numnodes; col++)
		printk("%02d ", col);
	printk("\n");
	for (row = 0; row < numnodes; row++) {
		printk("%02d  ", row);
		for (col = 0; col < numnodes; col++)
			printk("%2d ", node_distances[row][col]);
		printk("\n");
	}

	for (cnode = 0; cnode < numnodes; cnode++) {
		nasid = COMPACT_TO_NASID_NODEID(cnode);

		if (nasid == -1) continue;

		brd = find_lboard_class((lboard_t *)KL_CONFIG_INFO(nasid),
					KLTYPE_ROUTER);

		if (!brd)
			continue;

		do {
			if (brd->brd_flags & DUPLICATE_BOARD)
				continue;
			printk("Router %d:", router_num);
			router_num++;

			router = (klrou_t *)NODE_OFFSET_TO_K0(NASID_GET(brd), brd->brd_compts[0]);

			for (port = 1; port <= MAX_ROUTER_PORTS; port++) {
				if (router->rou_port[port].port_nasid == INVALID_NASID)
					continue;

				dest_brd = (lboard_t *)NODE_OFFSET_TO_K0(
					router->rou_port[port].port_nasid,
					router->rou_port[port].port_offset);

				if (dest_brd->brd_type == KLTYPE_IP27)
					printk(" %d", dest_brd->brd_nasid);
				if (dest_brd->brd_type == KLTYPE_ROUTER)
					printk(" r");
			}
			printk("\n");

		} while ( (brd = find_lboard_class(KLCF_NEXT(brd), KLTYPE_ROUTER)) );
	}
}
