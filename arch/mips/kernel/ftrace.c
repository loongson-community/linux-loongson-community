/*
 * Code for replacing ftrace calls with jumps.
 *
 * Copyright (C) 2007-2008 Steven Rostedt <srostedt@redhat.com>
 * Copyright (C) 2009, 2010 DSLab, Lanzhou University, China
 * Author: Wu Zhangjin <wuzhangjin@gmail.com>
 *
 * Thanks goes to Steven Rostedt for writing the original x86 version.
 */

#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/ftrace.h>

#include <asm/cacheflush.h>
#include <asm/asm.h>
#include <asm/asm-offsets.h>
#include <asm/uasm.h>

#define INSN_S_R_SP	0xafb00000	/* s{d,w} R, offset(sp) */

#ifdef CONFIG_DYNAMIC_FTRACE

/* Before linking, the following instructions are fixed. */
#ifdef CONFIG_CPU_LOONGSON2F
#define INSN_NOP 0x00200825	/* or at, at, zero */
#else
#define INSN_NOP 0x00000000	/* nop */
#endif
#define INSN_B_1F_16 0x10000004	/* b 1f; offset = (16 >> 2) */
#define INSN_B_1F_20 0x10000005	/* b 1f; offset = (20 >> 2) */

/* After linking, the following instructions are fixed. */
static unsigned int insn_jal_ftrace_caller __read_mostly;
static unsigned int insn_lui_v1_hi16_mcount __read_mostly;
static unsigned int insn_j_ftrace_graph_caller __maybe_unused __read_mostly;

/* The following instructions are encoded in the run-time */
/* insn: jal func; op_code|addr : 31...26|25 ....0 */
#define INSN_JAL(addr) \
	((unsigned int)(0x0c000000 | (((addr) >> 2) & 0x03ffffff)))

static inline void ftrace_dyn_arch_init_insns(void)
{
	u32 *buf;
	unsigned int v1;

	/* lui v1, hi16_mcount */
	v1 = 3;
	buf = (u32 *)&insn_lui_v1_hi16_mcount;
	UASM_i_LA_mostly(&buf, v1, MCOUNT_ADDR);

	/* jal (ftrace_caller + 8), jump over the first two instruction */
	buf = (u32 *)&insn_jal_ftrace_caller;
	uasm_i_jal(&buf, (FTRACE_ADDR + 8));

#ifdef CONFIG_FUNCTION_GRAPH_TRACER
	/* j ftrace_graph_caller */
	buf = (u32 *)&insn_j_ftrace_graph_caller;
	uasm_i_j(&buf, (unsigned long)ftrace_graph_caller);
#endif
}

static int ftrace_modify_code(unsigned long ip, unsigned int new_code)
{
	int faulted;

	/* *(unsigned int *)ip = new_code; */
	safe_store_code(new_code, ip, faulted);
	if (unlikely(faulted))
		return -EFAULT;

	flush_icache_range(ip, ip + 8);

	return 0;
}

int ftrace_make_nop(struct module *mod,
		    struct dyn_ftrace *rec, unsigned long addr)
{
	unsigned int new;
	unsigned long ip = rec->ip;

	/*
	 * We have compiled modules with -mlong-calls, but compiled kernel
	 * without it, therefore, need to cope with them respectively.
	 *
	 * For module:
	 *
	 *	lui	v1, hi16_mcount		--> b	1f
	 *	addiu	v1, v1, lo16_mcount
	 *	move at, ra
	 *	jalr v1
	 *	 nop
	 *					1f: (ip + 16)
	 * For kernel:
	 *
	 *	move	at, ra
	 *	jal	_mcount			--> nop
	 *
	 * And with the -mmcount-ra-address option, the offset may be 20 for
	 * leaf fuction and 24 for non-leaf function.
	 */

	if (!in_module(ip))
		new = INSN_NOP;
	else {
#if defined(KBUILD_MCOUNT_RA_ADDRESS) && defined(CONFIG_32BIT)
		new = INSN_B_1F_20;
#else
		new = INSN_B_1F_16;
#endif
	}

	return ftrace_modify_code(ip, new);
}

int ftrace_make_call(struct dyn_ftrace *rec, unsigned long addr)
{
	unsigned int new;
	unsigned long ip = rec->ip;

	new = in_module(ip) ? insn_lui_v1_hi16_mcount : insn_jal_ftrace_caller;

	return ftrace_modify_code(ip, new);
}

int ftrace_update_ftrace_func(ftrace_func_t func)
{
	unsigned int new;

	new = INSN_JAL((unsigned long)func);

	return ftrace_modify_code((unsigned long)(&ftrace_call), new);
}

int __init ftrace_dyn_arch_init(void *data)
{
	ftrace_dyn_arch_init_insns();

	/*
	 * We are safe to remove the "b ftrace_stub" for the current
	 * ftrace_caller() is almost empty (only the stack operations), and
	 * most importantly, the calling to mcount will be disabled later in
	 * ftrace_init(), then there is no 'big' overhead. And in the future,
	 * we are hoping the function_trace_stop is initialized as 1 then we
	 * can eventually remove that 'useless' "b ftrace_stub" and the
	 * following nop. Currently, although we can call ftrace_stop() to set
	 * function_trace_stop as 1, but it will change the behavior of the
	 * Function Tracer.
	 */
	ftrace_modify_code(MCOUNT_ADDR, INSN_NOP);

	/* The return code is retured via data */
	*(unsigned long *)data = 0;

	return 0;
}
#endif				/* CONFIG_DYNAMIC_FTRACE */

#ifdef CONFIG_FUNCTION_GRAPH_TRACER

#ifdef CONFIG_DYNAMIC_FTRACE
extern void ftrace_graph_call(void);
#define FTRACE_GRAPH_CALL_IP	((unsigned long)(&ftrace_graph_call))

int ftrace_enable_ftrace_graph_caller(void)
{
	return ftrace_modify_code(FTRACE_GRAPH_CALL_IP,
			insn_j_ftrace_graph_caller);
}

int ftrace_disable_ftrace_graph_caller(void)
{
	return ftrace_modify_code(FTRACE_GRAPH_CALL_IP, INSN_NOP);
}
#endif				/* !CONFIG_DYNAMIC_FTRACE */

#ifndef KBUILD_MCOUNT_RA_ADDRESS
#define INSN_S_RA_SP	0xafbf0000	/* s{d,w} ra, offset(sp) */
#define STACK_OFFSET_MASK	0xffff	/* stack offset range: 0 ~ PT_SIZE */

unsigned long ftrace_get_parent_addr(unsigned long self_addr,
				     unsigned long parent,
				     unsigned long parent_addr,
				     unsigned long fp)
{
	unsigned long sp, ip, ra;
	unsigned int code;
	int faulted;

	/*
	 * For module, move the ip from calling site of mcount to the
	 * instruction "lui v1, hi_16bit_of_mcount"(offset is 20), but for
	 * kernel, move to the instruction "move ra, at"(offset is 12)
	 */
	ip = self_addr - (in_module(self_addr) ? 20 : 12);

	/*
	 * search the text until finding the non-store instruction or "s{d,w}
	 * ra, offset(sp)" instruction
	 */
	do {
		ip -= 4;

		/* get the code at "ip": code = *(unsigned int *)ip; */
		safe_load_code(code, ip, faulted);
		if (unlikely(faulted))
			return 0;

		/*
		 * If we hit the non-store instruction before finding where the
		 * ra is stored, then this is a leaf function and it does not
		 * store the ra on the stack.
		 */
		if ((code & INSN_S_R_SP) != INSN_S_R_SP)
			return parent_addr;
	} while (((code & INSN_S_RA_SP) != INSN_S_RA_SP));

	sp = fp + (code & STACK_OFFSET_MASK);

	/* ra = *(unsigned long *)sp; */
	safe_load_stack(ra, sp, faulted);
	if (unlikely(faulted))
		return 0;

	if (ra == parent)
		return sp;
	return 0;
}
#endif	/* !KBUILD_MCOUNT_RA_ADDRESS */

/*
 * Hook the return address and push it in the stack of return addrs
 * in current thread info.
 */
void prepare_ftrace_return(unsigned long *parent, unsigned long self_addr,
			   unsigned long fp)
{
	int faulted;
	unsigned long old;
	struct ftrace_graph_ent trace;
	unsigned long return_hooker = (unsigned long)
	    &return_to_handler;

	if (unlikely(atomic_read(&current->tracing_graph_pause)))
		return;

	/*
	 * "parent" is the stack address saved the return address of the caller
	 * of _mcount.
	 *
	 * if the gcc < 4.5, a leaf function does not save the return address
	 * in the stack address, so, we "emulate" one in _mcount's stack space,
	 * and hijack it directly, but for a non-leaf function, it save the
	 * return address to the its own stack space, we can not hijack it
	 * directly, but need to find the real stack address,
	 * ftrace_get_parent_addr() does it!
	 *
	 * if gcc>= 4.5, with the new -mmcount-ra-address option, for a
	 * non-leaf function, the location of the return address will be saved
	 * to $12 for us, and for a leaf function, only put a zero into $12. we
	 * do it in ftrace_graph_caller of mcount.S.
	 */

	/* old = *parent; */
	safe_load_stack(old, parent, faulted);
	if (unlikely(faulted))
		goto out;
#ifndef KBUILD_MCOUNT_RA_ADDRESS
	parent = (unsigned long *)ftrace_get_parent_addr(
			self_addr, old, (unsigned long)parent, fp);

	/*
	 * If fails on getting the stack address of the non-leaf function's ra,
	 * stop function graph tracer and return
	 */
	if (unlikely(parent == 0))
		goto out;
#endif
	/* *parent = return_hooker; */
	safe_store_stack(return_hooker, parent, faulted);
	if (unlikely(faulted))
		goto out;

	if (ftrace_push_return_trace(old, self_addr, &trace.depth, fp) ==
	    -EBUSY) {
		*parent = old;
		return;
	}

	trace.func = self_addr;

	/* Only trace if the calling function expects to */
	if (!ftrace_graph_entry(&trace)) {
		current->curr_ret_stack--;
		*parent = old;
	}
	return;
out:
	ftrace_graph_stop();
	WARN_ON(1);
}
#endif				/* CONFIG_FUNCTION_GRAPH_TRACER */
