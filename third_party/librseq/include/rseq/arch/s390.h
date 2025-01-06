/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2018 Vasily Gorbik <gor@linux.ibm.com> */
/* SPDX-FileCopyrightText: 2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq-s390.h
 */

/*
 * RSEQ_ASM_*() macro helpers are internal to the librseq headers. Those
 * are not part of the public API.
 */

#ifndef _RSEQ_RSEQ_H
#error "Never use <rseq/arch/s390.h> directly; include <rseq/rseq.h> instead."
#endif

/*
 * RSEQ_SIG uses the trap4 instruction. As Linux does not make use of the
 * access-register mode nor the linkage stack this instruction will always
 * cause a special-operation exception (the trap-enabled bit in the DUCT
 * is and will stay 0). The instruction pattern is
 * 	b2 ff 0f ff	trap4	4095(%r0)
 */
#define RSEQ_SIG	0xB2FF0FFF

/*
 * Refer to the Linux kernel memory model (LKMM) for documentation of
 * the memory barriers.
 */

/* CPU memory barrier. */
#define rseq_smp_mb()	__asm__ __volatile__ ("bcr 15,0" ::: "memory")
/* CPU read memory barrier */
#define rseq_smp_rmb()	rseq_smp_mb()
/* CPU write memory barrier */
#define rseq_smp_wmb()	rseq_smp_mb()

/* Acquire: One-way permeable barrier. */
#define rseq_smp_load_acquire(p)					\
__extension__ ({							\
	rseq_unqual_scalar_typeof(*(p)) ____p1 = RSEQ_READ_ONCE(*(p));	\
	rseq_barrier();							\
	____p1;								\
})

/* Acquire barrier after control dependency. */
#define rseq_smp_acquire__after_ctrl_dep()	rseq_smp_rmb()

/* Release: One-way permeable barrier. */
#define rseq_smp_store_release(p, v)					\
do {									\
	rseq_barrier();							\
	RSEQ_WRITE_ONCE(*(p), v);					\
} while (0)

/*
 * Helper macros to access a variable of long integer type. Only used
 * internally in rseq headers.
 */
#ifdef RSEQ_ARCH_S390X
# define RSEQ_ASM_LONG_L		"lg"
# define RSEQ_ASM_LONG_S		"stg"
# define RSEQ_ASM_LONG_LT_R		"ltgr"
# define RSEQ_ASM_LONG_CMP		"cg"
# define RSEQ_ASM_LONG_CMP_R		"cgr"
# define RSEQ_ASM_LONG_ADDI		"aghi"
# define RSEQ_ASM_LONG_ADD_R		"agr"
#else
# define RSEQ_ASM_LONG_L		"l"
# define RSEQ_ASM_LONG_S		"st"
# define RSEQ_ASM_LONG_LT_R		"ltr"
# define RSEQ_ASM_LONG_CMP		"c"
# define RSEQ_ASM_LONG_CMP_R		"cr"
# define RSEQ_ASM_LONG_ADDI		"ahi"
# define RSEQ_ASM_LONG_ADD_R		"ar"
#endif

#ifdef RSEQ_ARCH_S390X
# define RSEQ_ASM_U64_PTR(x)		".quad " x
#else
/* 32-bit only supported on big endian. */
# define RSEQ_ASM_U64_PTR(x)		".long 0x0, " x
#endif

#define RSEQ_ASM_U32(x)			".long " x

/* Common architecture support macros. */
#include "rseq/arch/generic/common.h"

/*
 * Define a critical section abort handler.
 *
 *  @label:
 *    Local label to the abort handler.
 *  @teardown:
 *    Sequence of instructions to run on abort.
 *  @abort_label:
 *    C label to jump to at the end of the sequence.
 */
#define RSEQ_ASM_DEFINE_ABORT(label, teardown, abort_label)		\
		".pushsection __rseq_failure, \"ax\"\n\t"		\
		RSEQ_ASM_U32(__rseq_str(RSEQ_SIG)) "\n\t"		\
		__rseq_str(label) ":\n\t"				\
		teardown						\
		"jg %l[" __rseq_str(abort_label) "]\n\t"		\
		".popsection\n\t"

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
#define RSEQ_ASM_DEFINE_TEARDOWN(label, teardown, target_label)		\
		".pushsection __rseq_failure, \"ax\"\n\t"		\
		__rseq_str(label) ":\n\t"				\
		teardown						\
		"jg %l[" __rseq_str(target_label) "]\n\t"		\
		".popsection\n\t"

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
#define RSEQ_ASM_STORE_RSEQ_CS(label, cs_label, rseq_cs)		\
		RSEQ_INJECT_ASM(1)					\
		"larl %%r0, " __rseq_str(cs_label) "\n\t"		\
		RSEQ_ASM_LONG_S " %%r0, %[" __rseq_str(rseq_cs) "]\n\t"	\
		__rseq_str(label) ":\n\t"

/* Jump to local label @label when @cpu_id != @current_cpu_id. */
#define RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, label)		\
		RSEQ_INJECT_ASM(2)					\
		"c %[" __rseq_str(cpu_id) "], %[" __rseq_str(current_cpu_id) "]\n\t" \
		"jnz " __rseq_str(label) "\n\t"

/* Per-cpu-id indexing. */

#define RSEQ_TEMPLATE_INDEX_CPU_ID
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/s390/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED

#define RSEQ_TEMPLATE_MO_RELEASE
#include "rseq/arch/s390/bits.h"
#undef RSEQ_TEMPLATE_MO_RELEASE
#undef RSEQ_TEMPLATE_INDEX_CPU_ID

/* Per-mm-cid indexing. */

#define RSEQ_TEMPLATE_INDEX_MM_CID
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/s390/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED

#define RSEQ_TEMPLATE_MO_RELEASE
#include "rseq/arch/s390/bits.h"
#undef RSEQ_TEMPLATE_MO_RELEASE
#undef RSEQ_TEMPLATE_INDEX_MM_CID

/* APIs which are not indexed. */

#define RSEQ_TEMPLATE_INDEX_NONE
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/s390/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED
#undef RSEQ_TEMPLATE_INDEX_NONE
