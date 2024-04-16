/* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note */
/* SPDX-FileCopyrightText: 2015-2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */
#ifndef _RSEQ_ABI_H
#define _RSEQ_ABI_H

/*
 * rseq/rseq-abi.h
 *
 * Restartable sequences system call API
 */

#include <linux/types.h>
#include <asm/byteorder.h>

enum rseq_abi_cpu_id_state {
	RSEQ_ABI_CPU_ID_UNINITIALIZED		= -1,
	RSEQ_ABI_CPU_ID_REGISTRATION_FAILED		= -2,
};

enum rseq_abi_flags {
	RSEQ_ABI_FLAG_UNREGISTER = (1 << 0),
};

enum rseq_abi_cs_flags_bit {
	RSEQ_ABI_CS_FLAG_NO_RESTART_ON_PREEMPT_BIT	= 0,
	RSEQ_ABI_CS_FLAG_NO_RESTART_ON_SIGNAL_BIT	= 1,
	RSEQ_ABI_CS_FLAG_NO_RESTART_ON_MIGRATE_BIT	= 2,
};

enum rseq_abi_cs_flags {
	RSEQ_ABI_CS_FLAG_NO_RESTART_ON_PREEMPT	=
		(1U << RSEQ_ABI_CS_FLAG_NO_RESTART_ON_PREEMPT_BIT),
	RSEQ_ABI_CS_FLAG_NO_RESTART_ON_SIGNAL	=
		(1U << RSEQ_ABI_CS_FLAG_NO_RESTART_ON_SIGNAL_BIT),
	RSEQ_ABI_CS_FLAG_NO_RESTART_ON_MIGRATE	=
		(1U << RSEQ_ABI_CS_FLAG_NO_RESTART_ON_MIGRATE_BIT),
};

/*
 * struct rseq_abi_cs is aligned on 4 * 8 bytes to ensure it is always
 * contained within a single cache-line. It is usually declared as
 * link-time constant data.
 */
struct rseq_abi_cs {
	/* Version of this structure. */
	__u32 version;
	/* enum rseq_abi_cs_flags */
	__u32 flags;
	__u64 start_ip;
	/* Offset from start_ip. */
	__u64 post_commit_offset;
	__u64 abort_ip;
} __attribute__((aligned(4 * sizeof(__u64))));

/*
 * struct rseq is aligned on 4 * 8 bytes to ensure it is always
 * contained within a single cache-line.
 *
 * A single struct rseq per thread is allowed.
 */
struct rseq_abi {
	/*
	 * Restartable sequences cpu_id_start field. Updated by the
	 * kernel. Read by user-space with single-copy atomicity
	 * semantics. This field should only be read by the thread which
	 * registered this data structure. Aligned on 32-bit. Always
	 * contains a value in the range of possible CPUs, although the
	 * value may not be the actual current CPU (e.g. if rseq is not
	 * initialized). This CPU number value should always be compared
	 * against the value of the cpu_id field before performing a rseq
	 * commit or returning a value read from a data structure indexed
	 * using the cpu_id_start value.
	 */
	__u32 cpu_id_start;
	/*
	 * Restartable sequences cpu_id field. Updated by the kernel.
	 * Read by user-space with single-copy atomicity semantics. This
	 * field should only be read by the thread which registered this
	 * data structure. Aligned on 32-bit. Values
	 * RSEQ_ABI_CPU_ID_UNINITIALIZED and RSEQ_ABI_CPU_ID_REGISTRATION_FAILED
	 * have a special semantic: the former means "rseq uninitialized",
	 * and latter means "rseq initialization failed". This value is
	 * meant to be read within rseq critical sections and compared
	 * with the cpu_id_start value previously read, before performing
	 * the commit instruction, or read and compared with the
	 * cpu_id_start value before returning a value loaded from a data
	 * structure indexed using the cpu_id_start value.
	 */
	__u32 cpu_id;
	/*
	 * Restartable sequences rseq_cs field.
	 *
	 * Contains NULL when no critical section is active for the current
	 * thread, or holds a pointer to the currently active struct rseq_abi_cs.
	 *
	 * Updated by user-space, which sets the address of the currently
	 * active rseq_cs at the beginning of assembly instruction sequence
	 * block, and set to NULL by the kernel when it restarts an assembly
	 * instruction sequence block, as well as when the kernel detects that
	 * it is preempting or delivering a signal outside of the range
	 * targeted by the rseq_cs. Also needs to be set to NULL by user-space
	 * before reclaiming memory that contains the targeted struct rseq_abi_cs.
	 *
	 * Read and set by the kernel. Set by user-space with single-copy
	 * atomicity semantics. This field should only be updated by the
	 * thread which registered this data structure. Aligned on 64-bit.
	 *
	 * 32-bit architectures should update the low order bits of the
	 * rseq_cs field, leaving the high order bits initialized to 0.
	 */
	union {
		__u64 ptr64;

		/*
		 * The "arch" field provides architecture accessor for
		 * the ptr field based on architecture pointer size and
		 * endianness.
		 */
		struct {
#ifdef __LP64__
			__u64 ptr;
#elif defined(__BYTE_ORDER) ? (__BYTE_ORDER == __BIG_ENDIAN) : defined(__BIG_ENDIAN)
			__u32 padding;		/* Initialized to zero. */
			__u32 ptr;
#else
			__u32 ptr;
			__u32 padding;		/* Initialized to zero. */
#endif
		} arch;
	} rseq_cs;

	/*
	 * Restartable sequences flags field.
	 *
	 * This field should only be updated by the thread which
	 * registered this data structure. Read by the kernel.
	 * Mainly used for single-stepping through rseq critical sections
	 * with debuggers.
	 *
	 * - RSEQ_ABI_CS_FLAG_NO_RESTART_ON_PREEMPT
	 *     Inhibit instruction sequence block restart on preemption
	 *     for this thread.
	 * - RSEQ_ABI_CS_FLAG_NO_RESTART_ON_SIGNAL
	 *     Inhibit instruction sequence block restart on signal
	 *     delivery for this thread.
	 * - RSEQ_ABI_CS_FLAG_NO_RESTART_ON_MIGRATE
	 *     Inhibit instruction sequence block restart on migration for
	 *     this thread.
	 */
	__u32 flags;

	/*
	 * Restartable sequences node_id field. Updated by the kernel. Read by
	 * user-space with single-copy atomicity semantics. This field should
	 * only be read by the thread which registered this data structure.
	 * Aligned on 32-bit. Contains the current NUMA node ID.
	 */
	__u32 node_id;

	/*
	 * Restartable sequences mm_cid field. Updated by the kernel. Read by
	 * user-space with single-copy atomicity semantics. This field should
	 * only be read by the thread which registered this data structure.
	 * Aligned on 32-bit. Contains the current thread's concurrency ID
	 * (allocated uniquely within a memory map).
	 */
	__u32 mm_cid;

	/*
	 * Flexible array member at end of structure, after last feature field.
	 */
	char end[];
} __attribute__((aligned(4 * sizeof(__u64))));

/*
 * Define the rseq system call number if not yet available in
 * the system headers.
 */
#ifdef __x86_64__

#ifndef __NR_rseq
#define __NR_rseq		334
#endif

#elif defined(__i386__)

#ifndef __NR_rseq
#define __NR_rseq		386
#endif

#elif defined(__AARCH64EL__)

#ifndef __NR_rseq
#define __NR_rseq		293
#endif

#elif defined(__ARMEL__)

#ifndef __NR_rseq
#define __NR_rseq		398
#endif

#elif defined(__PPC__)

#ifndef __NR_rseq
#define __NR_rseq		387
#endif

#elif defined(__s390__)

#ifndef __NR_rseq
#define __NR_rseq		383
#endif

#elif defined(__riscv)

#ifndef __NR_rseq
#define __NR_rseq		293
#endif

#endif

#endif /* _RSEQ_ABI_H */
