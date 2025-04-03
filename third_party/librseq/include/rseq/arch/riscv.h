/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Vincent Chen <vincent.chen@sifive.com> */
/* SPDX-FileCopyrightText: 2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq-riscv.h
 */

/*
 * RSEQ_ASM_*() macro helpers are internal to the librseq headers. Those
 * are not part of the public API.
 */

#ifndef _RSEQ_RSEQ_H
#error "Never use <rseq/arch/riscv.h> directly; include <rseq/rseq.h> instead."
#endif

/*
 * Select the instruction "csrw mhartid, x0" as the RSEQ_SIG. Unlike
 * other architectures, the ebreak instruction has no immediate field for
 * distinguishing purposes. Hence, ebreak is not suitable as RSEQ_SIG.
 * "csrw mhartid, x0" can also satisfy the RSEQ requirement because it
 * is an uncommon instruction and will raise an illegal instruction
 * exception when executed in all modes.
 */
#include <endian.h>

#if defined(__BYTE_ORDER) ? (__BYTE_ORDER == __LITTLE_ENDIAN) : defined(__LITTLE_ENDIAN)
#define RSEQ_SIG   0xf1401073  /* csrr mhartid, x0 */
#else
#error "Currently, RSEQ only supports Little-Endian version"
#endif

/*
 * Instruction selection between 32-bit/64-bit. Used internally in the
 * rseq headers.
 */
#if __riscv_xlen == 64
#define __RSEQ_ASM_REG_SEL(a, b)	a
#elif __riscv_xlen == 32
#define __RSEQ_ASM_REG_SEL(a, b)	b
#endif

#define RSEQ_ASM_REG_L	__RSEQ_ASM_REG_SEL("ld ", "lw ")
#define RSEQ_ASM_REG_S	__RSEQ_ASM_REG_SEL("sd ", "sw ")

/*
 * Refer to the Linux kernel memory model (LKMM) for documentation of
 * the memory barriers.
 */

/* Only used internally in rseq headers. */
#define RSEQ_ASM_RISCV_FENCE(p, s) \
	__asm__ __volatile__ ("fence " #p "," #s : : : "memory")
/* CPU memory barrier. */
#define rseq_smp_mb()	RSEQ_ASM_RISCV_FENCE(rw, rw)
/* CPU read memory barrier */
#define rseq_smp_rmb()	RSEQ_ASM_RISCV_FENCE(r, r)
/* CPU write memory barrier */
#define rseq_smp_wmb()	RSEQ_ASM_RISCV_FENCE(w, w)

/* Acquire: One-way permeable barrier. */
#define rseq_smp_load_acquire(p)					\
__extension__ ({							\
	rseq_unqual_scalar_typeof(*(p)) ____p1 = RSEQ_READ_ONCE(*(p));	\
	RSEQ_ASM_RISCV_FENCE(r, rw);					\
	____p1;								\
})

/* Acquire barrier after control dependency. */
#define rseq_smp_acquire__after_ctrl_dep()	rseq_smp_rmb()

/* Release: One-way permeable barrier. */
#define rseq_smp_store_release(p, v)					\
do {									\
	RSEQ_ASM_RISCV_FENCE(rw, w);					\
	RSEQ_WRITE_ONCE(*(p), v);					\
} while (0)

#define RSEQ_ASM_U64_PTR(x)	".quad " x
#define RSEQ_ASM_U32(x)		".long " x

/* Temporary registers. */
#define RSEQ_ASM_TMP_REG_1	"t6"
#define RSEQ_ASM_TMP_REG_2	"t5"
#define RSEQ_ASM_TMP_REG_3	"t4"
#define RSEQ_ASM_TMP_REG_4	"t3"

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
	"j	222f\n"							\
	".balign	4\n"						\
	RSEQ_ASM_U32(__rseq_str(RSEQ_SIG)) "\n"				\
	__rseq_str(label) ":\n"						\
	teardown							\
	"j	%l[" __rseq_str(abort_label) "]\n"			\
	"222:\n"

/* Jump to local label @label when @cpu_id != @current_cpu_id. */
#define RSEQ_ASM_STORE_RSEQ_CS(label, cs_label, rseq_cs)		\
	RSEQ_INJECT_ASM(1)						\
	"la	" RSEQ_ASM_TMP_REG_1 ", " __rseq_str(cs_label) "\n"	\
	RSEQ_ASM_REG_S	RSEQ_ASM_TMP_REG_1 ", %[" __rseq_str(rseq_cs) "]\n" \
	__rseq_str(label) ":\n"

/* Store @value to address @var. */
#define RSEQ_ASM_OP_STORE(value, var)					\
	RSEQ_ASM_REG_S	"%[" __rseq_str(value) "], %[" __rseq_str(var) "]\n"

/* Jump to local label @label when @var != @expect. */
#define RSEQ_ASM_OP_CBNE(var, expect, label)				\
	RSEQ_ASM_REG_L	RSEQ_ASM_TMP_REG_1 ", %[" __rseq_str(var) "]\n"	\
	"bne	" RSEQ_ASM_TMP_REG_1 ", %[" __rseq_str(expect) "] ,"	\
		  __rseq_str(label) "\n"

/*
 * Jump to local label @label when @var != @expect (32-bit register
 * comparison).
 */
#define RSEQ_ASM_OP_CBNE32(var, expect, label)				\
	"lw	" RSEQ_ASM_TMP_REG_1 ", %[" __rseq_str(var) "]\n"	\
	"bne	" RSEQ_ASM_TMP_REG_1 ", %[" __rseq_str(expect) "] ,"	\
		  __rseq_str(label) "\n"

/* Jump to local label @label when @var == @expect. */
#define RSEQ_ASM_OP_CBEQ(var, expect, label)				\
	RSEQ_ASM_REG_L	RSEQ_ASM_TMP_REG_1 ", %[" __rseq_str(var) "]\n"	\
	"beq	" RSEQ_ASM_TMP_REG_1 ", %[" __rseq_str(expect) "] ,"	\
		  __rseq_str(label) "\n"

/* Jump to local label @label when @cpu_id != @current_cpu_id. */
#define RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, label)		\
	RSEQ_INJECT_ASM(2)						\
	RSEQ_ASM_OP_CBNE32(current_cpu_id, cpu_id, label)

/* Load @var into temporary register. */
#define RSEQ_ASM_OP_R_LOAD(var)						\
	RSEQ_ASM_REG_L	RSEQ_ASM_TMP_REG_1 ", %[" __rseq_str(var) "]\n"

/* Store from temporary register into @var. */
#define RSEQ_ASM_OP_R_STORE(var)					\
	RSEQ_ASM_REG_S	RSEQ_ASM_TMP_REG_1 ", %[" __rseq_str(var) "]\n"

/* Load from address in temporary register+@offset into temporary register. */
#define RSEQ_ASM_OP_R_LOAD_OFF(offset)					\
	"add	" RSEQ_ASM_TMP_REG_1 ", %[" __rseq_str(offset) "], "	\
		 RSEQ_ASM_TMP_REG_1 "\n"				\
	RSEQ_ASM_REG_L	RSEQ_ASM_TMP_REG_1 ", (" RSEQ_ASM_TMP_REG_1 ")\n"

/* Add @count to temporary register. */
#define RSEQ_ASM_OP_R_ADD(count)					\
	"add	" RSEQ_ASM_TMP_REG_1 ", " RSEQ_ASM_TMP_REG_1		\
		", %[" __rseq_str(count) "]\n"

/*
 * End-of-sequence store of @value to address @var. Emit
 * @post_commit_label label after the store instruction.
 */
#define RSEQ_ASM_OP_FINAL_STORE(value, var, post_commit_label)		\
	RSEQ_ASM_OP_STORE(value, var)					\
	__rseq_str(post_commit_label) ":\n"

/*
 * End-of-sequence store-release of @value to address @var. Emit
 * @post_commit_label label after the store instruction.
 */
#define RSEQ_ASM_OP_FINAL_STORE_RELEASE(value, var, post_commit_label)	\
	"fence	rw, w\n"						\
	RSEQ_ASM_OP_STORE(value, var)					\
	__rseq_str(post_commit_label) ":\n"

/*
 * End-of-sequence store of temporary register to address @var. Emit
 * @post_commit_label label after the store instruction.
 */
#define RSEQ_ASM_OP_R_FINAL_STORE(var, post_commit_label)		\
	RSEQ_ASM_REG_S	RSEQ_ASM_TMP_REG_1 ", %[" __rseq_str(var) "]\n"	\
	__rseq_str(post_commit_label) ":\n"

/*
 * Copy @len bytes from @src to @dst. This is an inefficient bytewise
 * copy and could be improved in the future.
 */
#define RSEQ_ASM_OP_R_BYTEWISE_MEMCPY(dst, src, len)			\
	"beqz	%[" __rseq_str(len) "], 333f\n"				\
	"mv	" RSEQ_ASM_TMP_REG_1 ", %[" __rseq_str(len) "]\n"	\
	"mv	" RSEQ_ASM_TMP_REG_2 ", %[" __rseq_str(src) "]\n"	\
	"mv	" RSEQ_ASM_TMP_REG_3 ", %[" __rseq_str(dst) "]\n"	\
	"222:\n"							\
	"lb	" RSEQ_ASM_TMP_REG_4 ", 0(" RSEQ_ASM_TMP_REG_2 ")\n"	\
	"sb	" RSEQ_ASM_TMP_REG_4 ", 0(" RSEQ_ASM_TMP_REG_3 ")\n"	\
	"addi	" RSEQ_ASM_TMP_REG_1 ", " RSEQ_ASM_TMP_REG_1 ", -1\n"	\
	"addi	" RSEQ_ASM_TMP_REG_2 ", " RSEQ_ASM_TMP_REG_2 ", 1\n"	\
	"addi	" RSEQ_ASM_TMP_REG_3 ", " RSEQ_ASM_TMP_REG_3 ", 1\n"	\
	"bnez	" RSEQ_ASM_TMP_REG_1 ", 222b\n"				\
	"333:\n"

/* Per-cpu-id indexing. */

#define RSEQ_TEMPLATE_INDEX_CPU_ID
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/riscv/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED

#define RSEQ_TEMPLATE_MO_RELEASE
#include "rseq/arch/riscv/bits.h"
#undef RSEQ_TEMPLATE_MO_RELEASE
#undef RSEQ_TEMPLATE_INDEX_CPU_ID

/* Per-mm-cid indexing. */

#define RSEQ_TEMPLATE_INDEX_MM_CID
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/riscv/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED

#define RSEQ_TEMPLATE_MO_RELEASE
#include "rseq/arch/riscv/bits.h"
#undef RSEQ_TEMPLATE_MO_RELEASE
#undef RSEQ_TEMPLATE_INDEX_MM_CID

/* APIs which are not indexed. */

#define RSEQ_TEMPLATE_INDEX_NONE
#define RSEQ_TEMPLATE_MO_RELAXED
#include "rseq/arch/riscv/bits.h"
#undef RSEQ_TEMPLATE_MO_RELAXED
#undef RSEQ_TEMPLATE_INDEX_NONE
