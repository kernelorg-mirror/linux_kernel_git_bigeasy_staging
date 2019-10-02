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
#define smp_mb__after_spinlock()	smp_mb()

static inline void arch_spin_lock(arch_spinlock_t *lock)
{
	unsigned long tmp;
	u32 newval;
	arch_spinlock_t lockval;

	prefetchw(&lock->slock);
	__asm__ __volatile__(
"1:	ldrex	%0, [%3]\n"
"	add	%1, %0, %4\n"
"	strex	%2, %1, [%3]\n"
"	teq	%2, #0\n"
"	bne	1b"
	: "=&r" (lockval), "=&r" (newval), "=&r" (tmp)
	: "r" (&lock->slock), "I" (1 << TICKET_SHIFT)
	: "cc");

	while (lockval.tickets.next != lockval.tickets.owner) {
		wfe();
		lockval.tickets.owner = READ_ONCE(lock->tickets.owner);
	}

	smp_mb();
}

static inline int arch_spin_trylock(arch_spinlock_t *lock)
{
	unsigned long contended, res;
	u32 slock;

	prefetchw(&lock->slock);
	do {
		__asm__ __volatile__(
		"	ldrex	%0, [%3]\n"
		"	mov	%2, #0\n"
		"	subs	%1, %0, %0, ror #16\n"
		"	addeq	%0, %0, %4\n"
		"	strexeq	%2, %0, [%3]"
		: "=&r" (slock), "=&r" (contended), "=&r" (res)
		: "r" (&lock->slock), "I" (1 << TICKET_SHIFT)
		: "cc");
	} while (res);

	if (!contended) {
		smp_mb();
		return 1;
	} else {
		return 0;
	}
}

static inline void arch_spin_unlock(arch_spinlock_t *lock)
{
	smp_mb();
	lock->tickets.owner++;
	dsb_sev();
}

static inline int arch_spin_value_unlocked(arch_spinlock_t lock)
{
	return lock.tickets.owner == lock.tickets.next;
}

static inline int arch_spin_is_locked(arch_spinlock_t *lock)
{
	return !arch_spin_value_unlocked(READ_ONCE(*lock));
}

static inline int arch_spin_is_contended(arch_spinlock_t *lock)
{
	struct __raw_tickets tickets = READ_ONCE(lock->tickets);
	return (tickets.next - tickets.owner) > 1;
}
#define arch_spin_is_contended	arch_spin_is_contended


#endif /* __ASM_SPINLOCK_H */
