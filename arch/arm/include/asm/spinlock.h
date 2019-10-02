/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_SPINLOCK_H
#define __ASM_SPINLOCK_H

#if __LINUX_ARM_ARCH__ < 6
#error SMP not supported on pre-ARMv6 CPUs
#endif

#include <linux/prefetch.h>
#include <asm/barrier.h>
#include <asm/processor.h>

/*
 * sev and wfe are ARMv6K extensions.  Uniprocessor ARMv6 may not have the K
 * extensions, so when running on UP, we have to patch these instructions away.
 */
#ifdef CONFIG_THUMB2_KERNEL
/*
 * For Thumb-2, special care is needed to ensure that the conditional WFE
 * instruction really does assemble to exactly 4 bytes (as required by
 * the SMP_ON_UP fixup code).   By itself "wfene" might cause the
 * assembler to insert a extra (16-bit) IT instruction, depending on the
 * presence or absence of neighbouring conditional instructions.
 *
 * To avoid this unpredictableness, an approprite IT is inserted explicitly:
 * the assembler won't change IT instructions which are explicitly present
 * in the input.
 */
#define WFE(cond)	__ALT_SMP_ASM(		\
	"it " cond "\n\t"			\
	"wfe" cond ".n",			\
						\
	"nop.w"					\
)
#else
#define WFE(cond)	__ALT_SMP_ASM("wfe" cond, "nop")
#endif

#define SEV		__ALT_SMP_ASM(WASM(sev), WASM(nop))

static inline void dsb_sev(void)
{

	dsb(ishst);
	__asm__(SEV);
}

/*
 * ARMv6 ticket-based spin-locking.
 *
 * A memory barrier is required after we get a lock, and before we
 * release it, because V6 CPUs are assumed to have weakly ordered
 * memory.
 */

#include <asm/qrwlock.h>
#include <asm/qspinlock.h>
#define smp_mb__after_spinlock()	smp_mb()


#endif /* __ASM_SPINLOCK_H */
