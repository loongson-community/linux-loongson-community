#ifndef __ASM_MIPS_UNISTD_H
#define __ASM_MIPS_UNISTD_H

/* XXX - _foo needs to be __foo, while __NR_bar could be _NR_bar. */
#define _syscall0(type,name) \
type name(void) \
{ \
register long __res __asm__ ("$2"); \
__asm__ volatile ("syscall" \
                  : "=r" (__res) \
                  : "0" (__NR_##name)); \
if (__res >= 0) \
	return (type) __res; \
errno = -__res; \
return -1; \
}

#define _syscall1(type,name,atype,a) \
type name(atype a) \
{ \
register long __res __asm__ ("$2"); \
__asm__ volatile ("move\t$4,%2\n\t" \
                  "syscall" \
                  : "=r" (__res) \
                  : "0" (__NR_##name),"r" ((long)(a)) \
                  : "$4"); \
if (__res >= 0) \
	return (type) __res; \
errno = -__res; \
return -1; \
}

#define _syscall2(type,name,atype,a,btype,b) \
type name(atype a,btype b) \
{ \
register long __res __asm__ ("$2"); \
__asm__ volatile ("move\t$4,%2\n\t" \
                  "move\t$5,%3\n\t" \
                  "syscall" \
                  : "=r" (__res) \
                  : "0" (__NR_##name),"r" ((long)(a)), \
                                      "r" ((long)(b))); \
                  : "$4","$5"); \
if (__res >= 0) \
	return (type) __res; \
errno = -__res; \
return -1; \
}

#define _syscall3(type,name,atype,a,btype,b,ctype,c) \
type name (atype a, btype b, ctype c) \
{ \
register long __res __asm__ ("$2"); \
__asm__ volatile ("move\t$4,%2\n\t" \
                  "move\t$5,%3\n\t" \
                  "move\t$6,%4\n\t" \
                  "syscall" \
                  : "=r" (__res) \
                  : "0" (__NR_##name),"r" ((long)(a)), \
                                      "r" ((long)(b)), \
                                      "r" ((long)(c)) \
                  : "$4","$5","$6"); \
if (__res>=0) \
	return (type) __res; \
errno=-__res; \
return -1; \
}

#define _syscall4(type,name,atype,a,btype,b,ctype,c,dtype,d) \
type name (atype a, btype b, ctype c, dtype d) \
{ \
register long __res __asm__ ("$2"); \
__asm__ volatile ("move\t$4,%2\n\t" \
                  "move\t$5,%3\n\t" \
                  "move\t$6,%4\n\t" \
                  "move\t$7,%5\n\t" \
                  "syscall" \
                  : "=r" (__res) \
                  : "0" (__NR_##name),"r" ((long)(a)), \
                                      "r" ((long)(b)), \
                                      "r" ((long)(c)), \
                                      "r" ((long)(d)) \
                  : "$4","$5","$6","$7"); \
if (__res>=0) \
	return (type) __res; \
errno=-__res; \
return -1; \
}

#define _syscall5(type,name,atype,a,btype,b,ctype,c,dtype,d,etype,e) \
type name (atype a,btype b,ctype c,dtype d,etype e) \
{ \
register long __res __asm__ ("$2"); \
__asm__ volatile ("move\t$4,%2\n\t" \
                  "move\t$5,%3\n\t" \
                  "move\t$6,%4\n\t" \
                  "move\t$7,%5\n\t" \
                  "move\t$3,%6\n\t" \
                  "syscall" \
                  : "=r" (__res) \
                  : "0" (__NR_##name),"r" ((long)(a)), \
                                      "r" ((long)(b)), \
                                      "r" ((long)(c)), \
                                      "r" ((long)(d)), \
                                      "r" ((long)(e)) \
                  : "$3","$4","$5","$6","$7"); \
if (__res>=0) \
	return (type) __res; \
errno=-__res; \
return -1; \
}

#ifdef __KERNEL_SYSCALLS__

/*
 * we need this inline - forking from kernel space will result
 * in NO COPY ON WRITE (!!!), until an execve is executed. This
 * is no problem, but for the stack. This is handled by not letting
 * main() use the stack at all after fork(). Thus, no function
 * calls - which means inline code for fork too, as otherwise we
 * would use the stack upon exit from 'fork()'.
 *
 * Actually only pause and fork are needed inline, so that there
 * won't be any messing with the stack from main(), but we define
 * some others too.
 */
#define __NR__exit __NR_exit
static inline _syscall0(int,idle)
static inline _syscall0(int,fork)
static inline _syscall0(int,pause)
static inline _syscall0(int,setup)
static inline _syscall0(int,sync)
static inline _syscall0(pid_t,setsid)
static inline _syscall3(int,write,int,fd,const char *,buf,off_t,count)
static inline _syscall1(int,dup,int,fd)
static inline _syscall3(int,execve,const char *,file,char **,argv,char **,envp)
static inline _syscall3(int,open,const char *,file,int,flag,int,mode)
static inline _syscall1(int,close,int,fd)
static inline _syscall1(int,_exit,int,exitcode)
static inline _syscall3(pid_t,waitpid,pid_t,pid,int *,wait_stat,int,options)

static inline pid_t wait(int * wait_stat)
{
	return waitpid(-1,wait_stat,0);
}

#endif

#endif /* __ASM_MIPS_UNISTD_H */
