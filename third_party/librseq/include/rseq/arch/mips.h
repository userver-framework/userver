/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2018 MIPS Tech LLC */
/* SPDX-FileCopyrightText: 2016-2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * Author: Paul Burton <paul.burton@mips.com>
 */

#ifndef _RSEQ_RSEQ_H
#error "Never use <rseq/arch/mips.h> directly; include <rseq/rseq.h> instead."
#endif

#include <asm/byteorder.h>

/*
 * RSEQ_ASM_*() macro helpers are internal to the librseq headers. Those
 * are not part of the public API.
 */

#if (RSEQ_BITS_PER_LONG != 64) && (RSEQ_BITS_PER_LONG != 32)
# error unsupported RSEQ_BITS_PER_LONG
#endif

/*
 * RSEQ_SIG uses the break instruction. The instruction pattern is:
 *
 * On MIPS:
 *	0350000d        break     0x350
 *
 * On nanoMIPS:
 *      00100350        break     0x350
 *
 * On microMIPS:
 *      0000d407        break     0x350
 *
 * For nanoMIPS32 and microMIPS, the instruction stream is encoded as 16-bit
 * halfwords, so the signature halfwords need to be swapped accordingly for
 * little-endian.
 */
#if defined(__nanomips__)
# ifdef __MIPSEL__
#  define RSEQ_SIG	0x03500010
# else
#  define RSEQ_SIG	0x00100350
# endif
#elif defined(__mips_micromips)
# ifdef __MIPSEL__
#  define RSEQ_SIG	0xd4070000
# else
#  define RSEQ_SIG	0x0000d407
# endif
#elif defined(__mips__)
# define RSEQ_SIG	0x0350000d
#else
/* Unknown MIPS architecture. */
#endif

/*
 * Refer to the Linux kernel memory model (LKMM) for documentation of
 * the memory barriers.
 */

/* CPU memory barrier. */
#define rseq_smp_mb()	__asm__ __volatile__ ("sync" ::: "memory")
/* CPU read memory barrier */
#define rseq_smp_rmb()	rseq_smp_mb()
/* CPU write memory barrier */
#define rseq_smp_wmb()	rseq_smp_mb()

/* Acquire: One-way permeable barrier. */
#define rseq_smp_load_acquire(p)					\
__extension__ ({							\
	rseq_unqual_scalar_typeof(*(p)) ____p1 = RSEQ_READ_ONCE(*(p));	\
	rseq_smp_mb();							\
	____p1;								\
})

/* Acquire barrier after control dependency. */
#define rseq_smp_acquire__after_ctrl_dep()	rseq_smp_rmb()

/* Release: One-way permeable barrier. */
#define rseq_smp_store_release(p, v)					\
do {									\
	rseq_smp_mb();							\
	RSEQ_WRITE_ONCE(*(p), v);					\
} while (0)

/*
 * Helper macros to define and access a variable of long integer type.
 * Only used internally in rseq headers.
 */
#if RSEQ_BITS_PER_LONG == 64
# define RSEQ_ASM_LONG			".dword"
# define RSEQ_ASM_LONG_LA		"dla"
# define RSEQ_ASM_LONG_L		"ld"
# define RSEQ_ASM_LONG_S		"sd"
# define RSEQ_ASM_LONG_ADDI		"daddiu"
#else
# define RSEQ_ASM_LONG			".word"
# define RSEQ_ASM_LONG_LA		"la"
# define RSEQ_ASM_LONG_L		"lw"
# define RSEQ_ASM_LONG_S		"sw"
# define RSEQ_ASM_LONG_ADDI		"addiu"
#endif

/*
 * Helper macros to define a variable of pointer type stored in a 64-bit
 * integer. Only used internally in rseq headers.
 */
#if RSEQ_BITS_PER_LONG == 64
# define RSEQ_ASM_U64_PTR(x)		".dword " x
#else
# if defined(__BYTE_ORDER) ? (__BYTE_ORDER == __BIG_ENDIAN) : defined(__BIG_ENDIAN)
#  define RSEQ_ASM_U64_PTR(x)		".word 0x0, " x
# else
#  define RSEQ_ASM_U64_PTR(x)		".word " x ", 0x0"
# endif
#endif

#define RSEQ_ASM_U32(x)			".word " x

/* Common architecture support macros. */
#include "rseq/arch/generic/common.h"

/* Only used in RSEQ_ASM_DEFINE_ABORT. */
#define __RSEQ_ASM_DEFINE_ABORT(label, teardown, abort_label, \
				table_label, version, flags, \
				start_ip, post_commit_offset, abort_ip) \
		".balign 32\n\t" \
		__rseq_str(table_label) ":\n\t" \
		__RSEQ_ASM_DEFINE_CS_FIELDS(version, flags, \
			start_ip, post_commit_offset, abort_ip) "\n\t" \
		RSEQ_ASM_U32(__rseq_str(RSEQ_SIG)) "\n\t" \
		__rseq_str(label) ":\n\t" \
		teardown \
		"b %l[" __rseq_str(abort_label) "]\n\t"

/*
 * Define a critical section abort handler.
 *
 *  @label:
 *    Local label to the abort handler.
 *  @teardown:
 *    Sequence of instructions to run on abort.
 *  @abort_label:
 *    C label to jump to at the end of the sequence.
 *  @table_label:
 *    Local label to the critical section descriptor copy placed near
 *    the program counter. This is done for performance reasons because
 *    computing this address is faster than accessing the program data.
 *
 * The purpose of @start_ip, @post_commit_ip, and @abort_ip are
 * documented in RSEQ_ASM_DEFINE_TABLE.
 */
#define RSEQ_ASM_DEFINE_ABORT(label, teardown, abort_label, \
			      table_label, start_ip, post_commit_ip, abort_ip) \
	__RSEQ_ASM_DEFINE_ABORT(label, teardown, abort_label, \
				table_label, 0x0, 0x0, start_ip, \
				(post_commit_ip) - (start_ip), abort_ip)

/*
 * Define a critical section teardown handler.
 *
 *  @label:
 *    Local label to the teardown handler.
 *  @teardown:
 *    Sequence of instructions to run on teardown.
 *  @target_label:
 *    C label to jump to at the end of the sequence.
 */
#define RSEQ_ASM_DEFINE_TEARDOWN(label, teardown, target_label) \
		__rseq_str(label) ":\n\t" \
		teardown \
		"b %l[" __rseq_str(target_label) "]\n\t"

/*
 * Store the address of the critical section descriptor structure at
 * @cs_label into the @rseq_cs pointer and emit the label @label, which
 * is the beginning of the sequence of consecutive assembly instructions.
 *
 *  @label:
 *    Local label to the beginning of the sequence of consecutive assembly
 *    instructions.
 *  @cs_label:
 *    Source local label to the critical section descriptor structure.
 *  @rseq_cs:
 *    Destination pointer where to store the address of the critical
 *    section descriptor structure.
 */
#define RSEQ_ASM_STORE_RSEQ_CS(label, cs_label, rseq_cs) \
		RSEQ_INJECT_ASM(1) \
		RSEQ_ASM_LONG_LA " $4, " __rseq_str(cs_label) "\n\t" \
		RSEQ_ASM_LONG_S  " $4, %[" __rseq_str(rseq_cs) "]\n\t" \
		__rseq_str(label) ":\n\t"

/* Jump to local label @label when @cpu_id != @current_cpu_id. */
#define RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, label) \
		RSEQ_INJECT_ASM(2) \
		"lw  $4, %[" __rseq_str(current_cpu_id) "]\n\t" \
		"bne $4, %[" __rseq_str(cpu_id) "], " __rseq_str(label) "\n\t"

/* Per-cpu-id indexing. */

#define RSEQ_TEMPLATE_INDEX_CPU_ID
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/mips/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED

#define RSEQ_TEMPLATE_MO_RELEASE
#include "rseq/arch/mips/bits.h"
#undef RSEQ_TEMPLATE_MO_RELEASE
#undef RSEQ_TEMPLATE_INDEX_CPU_ID

/* Per-mm-cid indexing. */

#define RSEQ_TEMPLATE_INDEX_MM_CID
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/mips/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED

#define RSEQ_TEMPLATE_MO_RELEASE
#include "rseq/arch/mips/bits.h"
#undef RSEQ_TEMPLATE_MO_RELEASE
#undef RSEQ_TEMPLATE_INDEX_MM_CID

/* APIs which are not indexed. */

#define RSEQ_TEMPLATE_INDEX_NONE
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/mips/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED
#undef RSEQ_TEMPLATE_INDEX_NONE
