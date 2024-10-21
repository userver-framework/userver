/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2016-2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq/arch/x86.h
 */

#ifndef _RSEQ_RSEQ_H
#error "Never use <rseq/arch/x86.h> directly; include <rseq/rseq.h> instead."
#endif

#include <stdint.h>

/*
 * RSEQ_ASM_*() macro helpers are internal to the librseq headers. Those
 * are not part of the public API.
 */

/*
 * RSEQ_SIG is used with the following reserved undefined instructions, which
 * trap in user-space:
 *
 * x86-32:    0f b9 3d 53 30 05 53      ud1    0x53053053,%edi
 * x86-64:    0f b9 3d 53 30 05 53      ud1    0x53053053(%rip),%edi
 */
#define RSEQ_SIG	0x53053053

/*
 * Due to a compiler optimization bug in gcc-8 with asm goto and TLS asm input
 * operands, we cannot use "m" input operands, and rather pass the __rseq_abi
 * address through a "r" input operand.
 */

/*
 * Offset of cpu_id, rseq_cs, and mm_cid fields in struct rseq. Those
 * are defined explicitly as macros to be used from assembly.
 */
#define RSEQ_ASM_CPU_ID_OFFSET		4
#define RSEQ_ASM_CS_OFFSET		8
#define RSEQ_ASM_MM_CID_OFFSET		24

/*
 * Refer to the Linux kernel memory model (LKMM) for documentation of
 * the memory barriers. Expect all x86 hardware to be x86-TSO (Total
 * Store Order).
 */

/* CPU memory barrier. */
#define rseq_smp_mb()	\
	__asm__ __volatile__ ("lock; addl $0,-128(%%rsp)" ::: "memory", "cc")
/* CPU read memory barrier */
#define rseq_smp_rmb()	rseq_barrier()
/* CPU write memory barrier */
#define rseq_smp_wmb()	rseq_barrier()

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

/* Segment selector for the thread pointer. */
#ifdef RSEQ_ARCH_AMD64
# define RSEQ_ASM_TP_SEGMENT		%%fs
#else
# define RSEQ_ASM_TP_SEGMENT		%%gs
#endif

/*
 * Helper macro to define a variable of pointer type stored in a 64-bit
 * integer. Only used internally in rseq headers.
 */
#ifdef RSEQ_ARCH_AMD64
# define RSEQ_ASM_U64_PTR(x)		".quad " x
#else
# define RSEQ_ASM_U64_PTR(x)		".long " x ", 0x0"
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
		/*							\
		 * Disassembler-friendly signature:			\
		 *   x86-32: ud1 <sig>,%edi				\
		 *   x86-64: ud1 <sig>(%rip),%edi			\
		 */							\
		".byte 0x0f, 0xb9, 0x3d\n\t"				\
		".long " __rseq_str(RSEQ_SIG) "\n\t"			\
		__rseq_str(label) ":\n\t"				\
		teardown						\
		"jmp %l[" __rseq_str(abort_label) "]\n\t"		\
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
		"jmp %l[" __rseq_str(target_label) "]\n\t"		\
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
#ifdef RSEQ_ARCH_AMD64
#define RSEQ_ASM_STORE_RSEQ_CS(label, cs_label, rseq_cs)		\
		RSEQ_INJECT_ASM(1)					\
		"leaq " __rseq_str(cs_label) "(%%rip), %%rax\n\t"	\
		"movq %%rax, " __rseq_str(rseq_cs) "\n\t"		\
		__rseq_str(label) ":\n\t"
#else
# define RSEQ_ASM_REF_LABEL	881
/*
 * Use ip-relative addressing to get the address to the rseq critical
 * section descriptor. On x86-32, this requires a "call" instruction to
 * get the instruction pointer, which modifies the stack. Beware of this
 * side-effect if this scheme is used within a rseq critical section.
 * This computation is performed immediately before storing the rseq_cs,
 * which is outside of the critical section.
 * Balance call/ret to help speculation.
 * Save this ip address to ref_ip for use by the critical section so
 * ip-relative addressing can be done without modifying the stack
 * pointer by using ref_ip and calculating the relative offset from
 * ref_label.
 */
# define RSEQ_ASM_STORE_RSEQ_CS(label, cs_label, rseq_cs, ref_ip, ref_label) \
		"jmp 779f\n\t"						\
		"880:\n\t"						\
		"movl (%%esp), %%eax\n\t"				\
		"ret\n\t"						\
		"779:\n\t"						\
		"call 880b\n\t"						\
		__rseq_str(ref_label) ":\n\t"				\
		"movl %%eax, " __rseq_str(ref_ip) "\n\t"		\
		"leal (" __rseq_str(cs_label) " - " __rseq_str(ref_label) "b)(%%eax), %%eax\n\t" \
		"movl %%eax, " __rseq_str(rseq_cs) "\n\t"		\
		__rseq_str(label) ":\n\t"
#endif

/* Jump to local label @label when @cpu_id != @current_cpu_id. */
#define RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, label)		\
		RSEQ_INJECT_ASM(2)					\
		"cmpl %[" __rseq_str(cpu_id) "], " __rseq_str(current_cpu_id) "\n\t" \
		"jnz " __rseq_str(label) "\n\t"

/* Per-cpu-id indexing. */

#define RSEQ_TEMPLATE_INDEX_CPU_ID
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/x86/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED

#define RSEQ_TEMPLATE_MO_RELEASE
#include "rseq/arch/x86/bits.h"
#undef RSEQ_TEMPLATE_MO_RELEASE
#undef RSEQ_TEMPLATE_INDEX_CPU_ID

/* Per-mm-cid indexing. */

#define RSEQ_TEMPLATE_INDEX_MM_CID
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/x86/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED

#define RSEQ_TEMPLATE_MO_RELEASE
#include "rseq/arch/x86/bits.h"
#undef RSEQ_TEMPLATE_MO_RELEASE
#undef RSEQ_TEMPLATE_INDEX_MM_CID

/* APIs which are not indexed. */

#define RSEQ_TEMPLATE_INDEX_NONE
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/x86/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED
#undef RSEQ_TEMPLATE_INDEX_NONE
