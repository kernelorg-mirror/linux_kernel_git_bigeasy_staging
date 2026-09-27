/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASMARM_BUG_H
#define _ASMARM_BUG_H

#include <linux/linkage.h>
#include <linux/types.h>
#include <asm/opcodes.h>

/*
 * Use a suitable undefined instruction to use for ARM/Thumb2 bug handling.
 * We need to be careful not to conflict with those used by other modules and
 * the register_undef_hook() system.
 */
#ifdef CONFIG_THUMB2_KERNEL
#define BUG_INSTR_VALUE 0xde02
#define BUG_INSTR(__value) __inst_thumb16(__value)
#define BUG_ENCODING() .short ___asm_opcode_to_mem_thumb16(BUG_INSTR_VALUE)
#else
#define BUG_INSTR_VALUE 0xe7f001f2
#define BUG_INSTR(__value) __inst_arm(__value)
#define BUG_ENCODING() .long ___asm_opcode_to_mem_arm(BUG_INSTR_VALUE)
#endif


#define BUG() _BUG(__FILE__, __LINE__, BUG_INSTR_VALUE)
#define _BUG(file, line, value) __BUG(file, line, value)

#ifdef CONFIG_DEBUG_BUGVERBOSE

#define _BUG_VERBOSE_LINE(__file, __line)			\
	.pushsection .rodata.str, "aMS", %progbits, 1;		\
	2: .asciz  __file;					\
	.popsection;						\
	3:							\
	.word 2b;						\
	.hword __line;						\

#define __BUG__LINE(__file, __line, __value)			\
	1: ;							\
	BUG_ENCODING();						\
	.pushsection __bug_table,"aw";				\
	.align 2;						\
	.word 1b;						\
	_BUG_VERBOSE_LINE(__file, __line);			\
	.hword 0;						\
	.popsection;

/*
 * The extra indirection is to ensure that the __FILE__ string comes through
 * OK. Many version of gcc do not support the asm %c parameter which would be
 * preferable to this unpleasantness. We use mergeable string sections to
 * avoid multiple copies of the string appearing in the kernel image.
 */
#define __BUG(__file, __line, __value)				\
do {								\
	asm volatile(__stringify(__BUG__LINE(__file, __line, __value))); \
	unreachable();						\
} while (0)

#else

#define __BUG(__file, __line, __value)				\
do {								\
	asm volatile(BUG_INSTR(__value) "\n");			\
	unreachable();						\
} while (0)
#endif  /* CONFIG_DEBUG_BUGVERBOSE */

#define HAVE_ARCH_BUG

#include <asm-generic/bug.h>

struct pt_regs;
void die(const char *msg, struct pt_regs *regs, int err);

void arm_notify_die(const char *str, struct pt_regs *regs,
		int signo, int si_code, void __user *addr,
		unsigned long err, unsigned long trap);

#ifdef CONFIG_ARM_LPAE
#define FAULT_CODE_ALIGNMENT	33
#define FAULT_CODE_DEBUG	34
#else
#define FAULT_CODE_ALIGNMENT	1
#define FAULT_CODE_DEBUG	2
#endif

void hook_fault_code(int nr, int (*fn)(unsigned long, unsigned int,
				       struct pt_regs *),
		     int sig, int code, const char *name);

void hook_ifault_code(int nr, int (*fn)(unsigned long, unsigned int,
				       struct pt_regs *),
		     int sig, int code, const char *name);

extern asmlinkage void c_backtrace(unsigned long fp, int pmode,
				   const char *loglvl);

struct mm_struct;
void show_pte(const char *lvl, struct mm_struct *mm, unsigned long addr);
extern void __show_regs(struct pt_regs *);
extern void __show_regs_alloc_free(struct pt_regs *regs);

#endif
