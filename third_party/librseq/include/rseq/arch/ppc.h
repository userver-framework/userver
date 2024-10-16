/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2016-2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */
/* SPDX-FileCopyrightText: 2016-2018 Boqun Feng <boqun.feng@gmail.com> */

/*
 * rseq/arch/ppc.h
 */

#ifndef _RSEQ_RSEQ_H
#error "Never use <rseq/arch/ppc.h> directly; include <rseq/rseq.h> instead."
#endif

/*
 * RSEQ_ASM_*() macro helpers are internal to the librseq headers. Those
 * are not part of the public API.
 */

/*
 * RSEQ_SIG is used with the following trap instruction:
 *
 * powerpc-be:    0f e5 00 0b           twui   r5,11
 * powerpc64-le:  0b 00 e5 0f           twui   r5,11
 * powerpc64-be:  0f e5 00 0b           twui   r5,11
 */

#define RSEQ_SIG	0x0fe5000b

/*
 * Refer to the Linux kernel memory model (LKMM) for documentation of
 * the memory barriers.
 */

/* CPU memory barrier. */
#define rseq_smp_mb()		__asm__ __volatile__ ("sync"	::: "memory", "cc")
/* Only used internally in this header. */
#define __rseq_smp_lwsync()	__asm__ __volatile__ ("lwsync"	::: "memory", "cc")
/* CPU read memory barrier */
#define rseq_smp_rmb()		__rseq_smp_lwsync()
/* CPU write memory barrier */
#define rseq_smp_wmb()		__rseq_smp_lwsync()

/* Acquire: One-way permeable barrier. */
#define rseq_smp_load_acquire(p)					\
__extension__ ({							\
	rseq_unqual_scalar_typeof(*(p)) ____p1 = RSEQ_READ_ONCE(*(p));	\
	__rseq_smp_lwsync();						\
	____p1;								\
})

/* Acquire barrier after control dependency. */
#define rseq_smp_acquire__after_ctrl_dep()	__rseq_smp_lwsync()

/* Release: One-way permeable barrier. */
#define rseq_smp_store_release(p, v)					\
do {									\
	__rseq_smp_lwsync();						\
	RSEQ_WRITE_ONCE(*(p), v);					\
} while (0)

/*
 * Helper macros to define and access a variable of long integer type.
 * Only used internally in rseq headers.
 */
#ifdef RSEQ_ARCH_PPC64
# define RSEQ_ASM_STORE_LONG(arg)	"std%U[" __rseq_str(arg) "]%X[" __rseq_str(arg) "] "	/* To memory ("m" constraint) */
# define RSEQ_ASM_STORE_INT(arg)	"stw%U[" __rseq_str(arg) "]%X[" __rseq_str(arg) "] "	/* To memory ("m" constraint) */
# define RSEQ_ASM_LOAD_LONG(arg)	"ld%U[" __rseq_str(arg) "]%X[" __rseq_str(arg) "] "	/* From memory ("m" constraint) */
# define RSEQ_ASM_LOAD_INT(arg)		"lwz%U[" __rseq_str(arg) "]%X[" __rseq_str(arg) "] "	/* From memory ("m" constraint) */
# define RSEQ_ASM_LOADX_LONG		"ldx "							/* From base register ("b" constraint) */
# define RSEQ_ASM_CMP_LONG		"cmpd "							/* Register-to-register comparison */
# define RSEQ_ASM_CMP_LONG_INT		"cmpdi "						/* Register-to-immediate comparison */
#else
# define RSEQ_ASM_STORE_LONG(arg)	"stw%U[" __rseq_str(arg) "]%X[" __rseq_str(arg) "] "	/* To memory ("m" constraint) */
# define RSEQ_ASM_STORE_INT(arg)	RSEQ_ASM_STORE_LONG(arg)				/* To memory ("m" constraint) */
# define RSEQ_ASM_LOAD_LONG(arg)	"lwz%U[" __rseq_str(arg) "]%X[" __rseq_str(arg) "] "	/* From memory ("m" constraint) */
# define RSEQ_ASM_LOAD_INT(arg)		RSEQ_ASM_LOAD_LONG(arg)					/* From memory ("m" constraint) */
# define RSEQ_ASM_LOADX_LONG		"lwzx "							/* From base register ("b" constraint) */
# define RSEQ_ASM_CMP_LONG		"cmpw "							/* Register-to-register comparison */
# define RSEQ_ASM_CMP_LONG_INT		"cmpwi "						/* Register-to-immediate comparison */
#endif

/*
 * Helper macros to define a variable of pointer type stored in a 64-bit
 * integer. Only used internally in rseq headers.
 */
#ifdef RSEQ_ARCH_PPC64
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
#define RSEQ_ASM_DEFINE_ABORT(label, teardown, abort_label)			\
		".pushsection __rseq_failure, \"ax\"\n\t"			\
		RSEQ_ASM_U32(__rseq_str(RSEQ_SIG)) "\n\t"			\
		__rseq_str(label) ":\n\t"					\
		teardown							\
		"b %l[" __rseq_str(abort_label) "]\n\t"				\
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
#ifdef RSEQ_ARCH_PPC64
# define RSEQ_ASM_STORE_RSEQ_CS(label, cs_label, rseq_cs)			\
		RSEQ_INJECT_ASM(1)						\
		"lis %%r17, (" __rseq_str(cs_label) ")@highest\n\t"		\
		"ori %%r17, %%r17, (" __rseq_str(cs_label) ")@higher\n\t"	\
		"rldicr %%r17, %%r17, 32, 31\n\t"				\
		"oris %%r17, %%r17, (" __rseq_str(cs_label) ")@high\n\t"	\
		"ori %%r17, %%r17, (" __rseq_str(cs_label) ")@l\n\t"		\
		"std %%r17, %[" __rseq_str(rseq_cs) "]\n\t"			\
		__rseq_str(label) ":\n\t"
#else
# define RSEQ_ASM_STORE_RSEQ_CS(label, cs_label, rseq_cs)			\
		RSEQ_INJECT_ASM(1)						\
		"lis %%r17, (" __rseq_str(cs_label) ")@ha\n\t"			\
		"addi %%r17, %%r17, (" __rseq_str(cs_label) ")@l\n\t"		\
		RSEQ_ASM_STORE_INT(rseq_cs) "%%r17, %[" __rseq_str(rseq_cs) "]\n\t" \
		__rseq_str(label) ":\n\t"
#endif

/* Jump to local label @label when @cpu_id != @current_cpu_id. */
#define RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, label)			\
		RSEQ_INJECT_ASM(2)						\
		RSEQ_ASM_LOAD_INT(current_cpu_id) "%%r17, %[" __rseq_str(current_cpu_id) "]\n\t" \
		"cmpw cr7, %[" __rseq_str(cpu_id) "], %%r17\n\t"		\
		"bne- cr7, " __rseq_str(label) "\n\t"

/*
 * RSEQ_ASM_OPs: asm operations for rseq. Only used internally by rseq headers.
 * 	RSEQ_ASM_OP_R_*: has hard-coded registers in it
 * 	RSEQ_ASM_OP_* (else): doesn't have hard-coded registers(unless cr7)
 */

/* Jump to local label @label when @var != @expect. */
#define RSEQ_ASM_OP_CBNE(var, expect, label)					\
		RSEQ_ASM_LOAD_LONG(var) "%%r17, %[" __rseq_str(var) "]\n\t"	\
		RSEQ_ASM_CMP_LONG "cr7, %%r17, %[" __rseq_str(expect) "]\n\t"	\
		"bne- cr7, " __rseq_str(label) "\n\t"

/* Jump to local label @label when @var == @expect. */
#define RSEQ_ASM_OP_CBEQ(var, expect, label)					\
		RSEQ_ASM_LOAD_LONG(var) "%%r17, %[" __rseq_str(var) "]\n\t"	\
		RSEQ_ASM_CMP_LONG "cr7, %%r17, %[" __rseq_str(expect) "]\n\t" \
		"beq- cr7, " __rseq_str(label) "\n\t"

/* Store @value to address @var. */
#define RSEQ_ASM_OP_STORE(value, var)						\
		RSEQ_ASM_STORE_LONG(var) "%[" __rseq_str(value) "], %[" __rseq_str(var) "]\n\t"

/* Load @var to r17 */
#define RSEQ_ASM_OP_R_LOAD(var)							\
		RSEQ_ASM_LOAD_LONG(var) "%%r17, %[" __rseq_str(var) "]\n\t"

/* Store r17 to @var */
#define RSEQ_ASM_OP_R_STORE(var)						\
		RSEQ_ASM_STORE_LONG(var) "%%r17, %[" __rseq_str(var) "]\n\t"

/* Add @count to r17 */
#define RSEQ_ASM_OP_R_ADD(count)						\
		"add %%r17, %[" __rseq_str(count) "], %%r17\n\t"

/* Load (r17 + voffp) to r17 */
#define RSEQ_ASM_OP_R_LOADX(voffp)						\
		RSEQ_ASM_LOADX_LONG "%%r17, %[" __rseq_str(voffp) "], %%r17\n\t"

/*
 * Copy @len bytes from @src to @dst. This is an inefficient bytewise
 * copy and could be improved in the future.
 */
#define RSEQ_ASM_OP_R_BYTEWISE_MEMCPY() \
		RSEQ_ASM_CMP_LONG_INT "%%r19, 0\n\t" \
		"beq 333f\n\t" \
		"addi %%r20, %%r20, -1\n\t" \
		"addi %%r21, %%r21, -1\n\t" \
		"222:\n\t" \
		"lbzu %%r18, 1(%%r20)\n\t" \
		"stbu %%r18, 1(%%r21)\n\t" \
		"addi %%r19, %%r19, -1\n\t" \
		RSEQ_ASM_CMP_LONG_INT "%%r19, 0\n\t" \
		"bne 222b\n\t" \
		"333:\n\t" \

/*
 * End-of-sequence store of r17 to address @var. Emit
 * @post_commit_label label after the store instruction.
 */
#define RSEQ_ASM_OP_R_FINAL_STORE(var, post_commit_label)			\
		RSEQ_ASM_STORE_LONG(var) "%%r17, %[" __rseq_str(var) "]\n\t"	\
		__rseq_str(post_commit_label) ":\n\t"

/*
 * End-of-sequence store of @value to address @var. Emit
 * @post_commit_label label after the store instruction.
 */
#define RSEQ_ASM_OP_FINAL_STORE(value, var, post_commit_label)			\
		RSEQ_ASM_STORE_LONG(var) "%[" __rseq_str(value) "], %[" __rseq_str(var) "]\n\t" \
		__rseq_str(post_commit_label) ":\n\t"

/* Per-cpu-id indexing. */

#define RSEQ_TEMPLATE_INDEX_CPU_ID
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/ppc/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED

#define RSEQ_TEMPLATE_MO_RELEASE
#include "rseq/arch/ppc/bits.h"
#undef RSEQ_TEMPLATE_MO_RELEASE
#undef RSEQ_TEMPLATE_INDEX_CPU_ID

/* Per-mm-cid indexing. */

#define RSEQ_TEMPLATE_INDEX_MM_CID
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/ppc/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED

#define RSEQ_TEMPLATE_MO_RELEASE
#include "rseq/arch/ppc/bits.h"
#undef RSEQ_TEMPLATE_MO_RELEASE
#undef RSEQ_TEMPLATE_INDEX_MM_CID

/* APIs which are not indexed. */

#define RSEQ_TEMPLATE_INDEX_NONE
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/ppc/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED
#undef RSEQ_TEMPLATE_INDEX_NONE
