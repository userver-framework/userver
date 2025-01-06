/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2016-2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */
/* SPDX-FileCopyrightText: 2018 Will Deacon <will.deacon@arm.com> */

/*
 * rseq/arch/aarch64.h
 */

#ifndef _RSEQ_RSEQ_H
#error "Never use <rseq/arch/aarch64.h> directly; include <rseq/rseq.h> instead."
#endif

/*
 * RSEQ_ASM_*() macro helpers are internal to the librseq headers. Those
 * are not part of the public API.
 */

/*
 * aarch64 -mbig-endian generates mixed endianness code vs data:
 * little-endian code and big-endian data. Ensure the RSEQ_SIG signature
 * matches code endianness.
 */
#define RSEQ_SIG_CODE	0xd428bc00	/* BRK #0x45E0.  */

#ifdef __AARCH64EB__	/* Big endian */
# define RSEQ_SIG_DATA	0x00bc28d4	/* BRK #0x45E0.  */
#else			/* Little endian */
# define RSEQ_SIG_DATA	RSEQ_SIG_CODE
#endif

#define RSEQ_SIG	RSEQ_SIG_DATA

/*
 * Refer to the Linux kernel memory model (LKMM) for documentation of
 * the memory barriers.
 */

/* CPU memory barrier. */
#define rseq_smp_mb()	__asm__ __volatile__ ("dmb ish" ::: "memory")
/* CPU read memory barrier */
#define rseq_smp_rmb()	__asm__ __volatile__ ("dmb ishld" ::: "memory")
/* CPU write memory barrier */
#define rseq_smp_wmb()	__asm__ __volatile__ ("dmb ishst" ::: "memory")

/* Acquire: One-way permeable barrier. */
#define rseq_smp_load_acquire(p)						\
__extension__ ({								\
	union { rseq_unqual_scalar_typeof(*(p)) __val; char __c[sizeof(*(p))]; } __u; \
	switch (sizeof(*(p))) {							\
	case 1:									\
		__asm__ __volatile__ ("ldarb %w0, %1"				\
			: "=r" (*(__u8 *)__u.__c)				\
			: "Q" (*(p)) : "memory");				\
		break;								\
	case 2:									\
		__asm__ __volatile__ ("ldarh %w0, %1"				\
			: "=r" (*(__u16 *)__u.__c)				\
			: "Q" (*(p)) : "memory");				\
		break;								\
	case 4:									\
		__asm__ __volatile__ ("ldar %w0, %1"				\
			: "=r" (*(__u32 *)__u.__c)				\
			: "Q" (*(p)) : "memory");				\
		break;								\
	case 8:									\
		__asm__ __volatile__ ("ldar %0, %1"				\
			: "=r" (*(__u64 *)__u.__c)				\
			: "Q" (*(p)) : "memory");				\
		break;								\
	}									\
	(rseq_unqual_scalar_typeof(*(p)))__u.__val;				\
})

/* Acquire barrier after control dependency. */
#define rseq_smp_acquire__after_ctrl_dep()	rseq_smp_rmb()

/* Release: One-way permeable barrier. */
#define rseq_smp_store_release(p, v)						\
do {										\
	union { rseq_unqual_scalar_typeof(*(p)) __val; char __c[sizeof(*(p))]; } __u = \
		{ .__val = (rseq_unqual_scalar_typeof(*(p))) (v) };		\
	switch (sizeof(*(p))) {							\
	case 1:									\
		__asm__ __volatile__ ("stlrb %w1, %0"				\
				: "=Q" (*(p))					\
				: "r" (*(__u8 *)__u.__c)			\
				: "memory");					\
		break;								\
	case 2:									\
		__asm__ __volatile__ ("stlrh %w1, %0"				\
				: "=Q" (*(p))					\
				: "r" (*(__u16 *)__u.__c)			\
				: "memory");					\
		break;								\
	case 4:									\
		__asm__ __volatile__ ("stlr %w1, %0"				\
				: "=Q" (*(p))					\
				: "r" (*(__u32 *)__u.__c)			\
				: "memory");					\
		break;								\
	case 8:									\
		__asm__ __volatile__ ("stlr %1, %0"				\
				: "=Q" (*(p))					\
				: "r" (*(__u64 *)__u.__c)			\
				: "memory");					\
		break;								\
	}									\
} while (0)

#define RSEQ_ASM_U64_PTR(x)	".quad " x
#define RSEQ_ASM_U32(x)		".long " x

/* Temporary scratch registers. */
#define RSEQ_ASM_TMP_REG32	"w15"
#define RSEQ_ASM_TMP_REG	"x15"
#define RSEQ_ASM_TMP_REG_2	"x14"

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
	"	b	222f\n"							\
	"	.inst 	"	__rseq_str(RSEQ_SIG_CODE) "\n"			\
	__rseq_str(label) ":\n"							\
	teardown								\
	"	b	%l[" __rseq_str(abort_label) "]\n"			\
	"222:\n"

/* Jump to local label @label when @cpu_id != @current_cpu_id. */
#define RSEQ_ASM_STORE_RSEQ_CS(label, cs_label, rseq_cs)			\
	RSEQ_INJECT_ASM(1)							\
	"	adrp	" RSEQ_ASM_TMP_REG ", " __rseq_str(cs_label) "\n"	\
	"	add	" RSEQ_ASM_TMP_REG ", " RSEQ_ASM_TMP_REG		\
			", :lo12:" __rseq_str(cs_label) "\n"			\
	"	str	" RSEQ_ASM_TMP_REG ", %[" __rseq_str(rseq_cs) "]\n"	\
	__rseq_str(label) ":\n"

/* Store @value to address @var. */
#define RSEQ_ASM_OP_STORE(value, var)						\
	"	str	%[" __rseq_str(value) "], %[" __rseq_str(var) "]\n"

/* Store-release @value to address @var. */
#define RSEQ_ASM_OP_STORE_RELEASE(value, var)					\
	"	stlr	%[" __rseq_str(value) "], %[" __rseq_str(var) "]\n"

/*
 * End-of-sequence store of @value to address @var. Emit
 * @post_commit_label label after the store instruction.
 */
#define RSEQ_ASM_OP_FINAL_STORE(value, var, post_commit_label)			\
	RSEQ_ASM_OP_STORE(value, var)						\
	__rseq_str(post_commit_label) ":\n"

/*
 * End-of-sequence store-release of @value to address @var. Emit
 * @post_commit_label label after the store instruction.
 */
#define RSEQ_ASM_OP_FINAL_STORE_RELEASE(value, var, post_commit_label)		\
	RSEQ_ASM_OP_STORE_RELEASE(value, var)					\
	__rseq_str(post_commit_label) ":\n"

/* Jump to local label @label when @var != @expect. */
#define RSEQ_ASM_OP_CBNE(var, expect, label)					\
	"	ldr	" RSEQ_ASM_TMP_REG ", %[" __rseq_str(var) "]\n"		\
	"	sub	" RSEQ_ASM_TMP_REG ", " RSEQ_ASM_TMP_REG		\
			", %[" __rseq_str(expect) "]\n"				\
	"	cbnz	" RSEQ_ASM_TMP_REG ", " __rseq_str(label) "\n"

/*
 * Jump to local label @label when @var != @expect (32-bit register
 * comparison).
 */
#define RSEQ_ASM_OP_CBNE32(var, expect, label)					\
	"	ldr	" RSEQ_ASM_TMP_REG32 ", %[" __rseq_str(var) "]\n"	\
	"	sub	" RSEQ_ASM_TMP_REG32 ", " RSEQ_ASM_TMP_REG32		\
			", %w[" __rseq_str(expect) "]\n"			\
	"	cbnz	" RSEQ_ASM_TMP_REG32 ", " __rseq_str(label) "\n"

/* Jump to local label @label when @var == @expect. */
#define RSEQ_ASM_OP_CBEQ(var, expect, label)					\
	"	ldr	" RSEQ_ASM_TMP_REG ", %[" __rseq_str(var) "]\n"		\
	"	sub	" RSEQ_ASM_TMP_REG ", " RSEQ_ASM_TMP_REG		\
			", %[" __rseq_str(expect) "]\n"				\
	"	cbz	" RSEQ_ASM_TMP_REG ", " __rseq_str(label) "\n"

/* Jump to local label @label when @cpu_id != @current_cpu_id. */
#define RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, label)			\
	RSEQ_INJECT_ASM(2)							\
	RSEQ_ASM_OP_CBNE32(current_cpu_id, cpu_id, label)

/* Load @var into temporary register. */
#define RSEQ_ASM_OP_R_LOAD(var)							\
	"	ldr	" RSEQ_ASM_TMP_REG ", %[" __rseq_str(var) "]\n"

/* Store from temporary register into @var. */
#define RSEQ_ASM_OP_R_STORE(var)						\
	"	str	" RSEQ_ASM_TMP_REG ", %[" __rseq_str(var) "]\n"

/* Load from address in temporary register+@offset into temporary register. */
#define RSEQ_ASM_OP_R_LOAD_OFF(offset)						\
	"	ldr	" RSEQ_ASM_TMP_REG ", [" RSEQ_ASM_TMP_REG		\
			", %[" __rseq_str(offset) "]]\n"

/* Add @count to temporary register. */
#define RSEQ_ASM_OP_R_ADD(count)						\
	"	add	" RSEQ_ASM_TMP_REG ", " RSEQ_ASM_TMP_REG		\
			", %[" __rseq_str(count) "]\n"

/*
 * End-of-sequence store of temporary register to address @var. Emit
 * @post_commit_label label after the store instruction.
 */
#define RSEQ_ASM_OP_R_FINAL_STORE(var, post_commit_label)			\
	"	str	" RSEQ_ASM_TMP_REG ", %[" __rseq_str(var) "]\n"		\
	__rseq_str(post_commit_label) ":\n"

/*
 * Copy @len bytes from @src to @dst. This is an inefficient bytewise
 * copy and could be improved in the future.
 */
#define RSEQ_ASM_OP_R_BYTEWISE_MEMCPY(dst, src, len)				\
	"	cbz	%[" __rseq_str(len) "], 333f\n"				\
	"	mov	" RSEQ_ASM_TMP_REG_2 ", %[" __rseq_str(len) "]\n"	\
	"222:	sub	" RSEQ_ASM_TMP_REG_2 ", " RSEQ_ASM_TMP_REG_2 ", #1\n"	\
	"	ldrb	" RSEQ_ASM_TMP_REG32 ", [%[" __rseq_str(src) "]"	\
			", " RSEQ_ASM_TMP_REG_2 "]\n"				\
	"	strb	" RSEQ_ASM_TMP_REG32 ", [%[" __rseq_str(dst) "]"	\
			", " RSEQ_ASM_TMP_REG_2 "]\n"				\
	"	cbnz	" RSEQ_ASM_TMP_REG_2 ", 222b\n"				\
	"333:\n"

/* Per-cpu-id indexing. */

#define RSEQ_TEMPLATE_INDEX_CPU_ID
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/aarch64/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED

#define RSEQ_TEMPLATE_MO_RELEASE
#include "rseq/arch/aarch64/bits.h"
#undef RSEQ_TEMPLATE_MO_RELEASE
#undef RSEQ_TEMPLATE_INDEX_CPU_ID

/* Per-mm-cid indexing. */

#define RSEQ_TEMPLATE_INDEX_MM_CID
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/aarch64/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED

#define RSEQ_TEMPLATE_MO_RELEASE
#include "rseq/arch/aarch64/bits.h"
#undef RSEQ_TEMPLATE_MO_RELEASE
#undef RSEQ_TEMPLATE_INDEX_MM_CID

/* APIs which are not indexed. */

#define RSEQ_TEMPLATE_INDEX_NONE
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/aarch64/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED
#undef RSEQ_TEMPLATE_INDEX_NONE
