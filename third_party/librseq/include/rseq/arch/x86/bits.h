/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2016-2018 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq/arch/x86/bits.h
 */

#include "rseq/arch/templates/bits.h"

/*
 * Refer to rseq/pseudocode.h for pseudo-code of the rseq critical
 * section helpers.
 */
#include "rseq/pseudocode.h"

#ifdef __x86_64__

#if defined(RSEQ_TEMPLATE_MO_RELAXED) && \
	(defined(RSEQ_TEMPLATE_INDEX_CPU_ID) || defined(RSEQ_TEMPLATE_INDEX_MM_CID))

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbne_store__ptr)(intptr_t *v, intptr_t expect, intptr_t newv, int cpu)
{
	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[ne])
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error2])
#endif
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]))
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
		"cmpq %[v], %[expect]\n\t"
		"jne %l[ne]\n\t"
		RSEQ_INJECT_ASM(4)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), %l[error1])
		"cmpq %[v], %[expect]\n\t"
		"jne %l[error2]\n\t"
#endif
		/* final store */
		"movq %[newv], %[v]\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(5)
		RSEQ_ASM_DEFINE_ABORT(4, "", abort)
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  [v]			"m" (*v),
		  [expect]		"r" (expect),
		  [newv]		"r" (newv)
		: "memory", "cc", "rax"
		  RSEQ_INJECT_CLOBBER
		: abort, ne
#ifdef RSEQ_COMPARE_TWICE
		  , error1, error2
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
ne:
	rseq_after_asm_goto();
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_after_asm_goto();
	rseq_bug("expected value comparison failed");
#endif
}

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbeq_store_add_load_store__ptr)(intptr_t *v, intptr_t expectnot,
			       long voffp, intptr_t *load, int cpu)
{
	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[eq])
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error2])
#endif
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]))
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
		"movq %[v], %%rbx\n\t"
		"cmpq %%rbx, %[expectnot]\n\t"
		"je %l[eq]\n\t"
		RSEQ_INJECT_ASM(4)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), %l[error1])
		"movq %[v], %%rbx\n\t"
		"cmpq %%rbx, %[expectnot]\n\t"
		"je %l[error2]\n\t"
#endif
		"movq %%rbx, %[load]\n\t"
		"addq %[voffp], %%rbx\n\t"
		"movq (%%rbx), %%rbx\n\t"
		/* final store */
		"movq %%rbx, %[v]\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(5)
		RSEQ_ASM_DEFINE_ABORT(4, "", abort)
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  /* final store input */
		  [v]			"m" (*v),
		  [expectnot]		"r" (expectnot),
		  [voffp]		"er" (voffp),
		  [load]		"m" (*load)
		: "memory", "cc", "rax", "rbx"
		  RSEQ_INJECT_CLOBBER
		: abort, eq
#ifdef RSEQ_COMPARE_TWICE
		  , error1, error2
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
eq:
	rseq_after_asm_goto();
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_after_asm_goto();
	rseq_bug("expected value comparison failed");
#endif
}

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_add_store__ptr)(intptr_t *v, intptr_t count, int cpu)
{
	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
#endif
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]))
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), %l[error1])
#endif
		/* final store */
		"addq %[count], %[v]\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(4)
		RSEQ_ASM_DEFINE_ABORT(4, "", abort)
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  /* final store input */
		  [v]			"m" (*v),
		  [count]		"er" (count)
		: "memory", "cc", "rax"
		  RSEQ_INJECT_CLOBBER
		: abort
#ifdef RSEQ_COMPARE_TWICE
		  , error1
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
#endif
}

#define rseq_arch_has_load_add_load_load_add_store

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_add_load_load_add_store__ptr)(intptr_t *ptr, long off, intptr_t inc, int cpu)
{
	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
#endif
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]))
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), %l[error1])
#endif
		/* get p+v */
		"movq %[ptr], %%rbx\n\t"
		"addq %[off], %%rbx\n\t"
		/* get pv */
		"movq (%%rbx), %%rcx\n\t"
		/* *pv += inc */
		"addq %[inc], (%%rcx)\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(4)
		RSEQ_ASM_DEFINE_ABORT(4, "", abort)
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  /* final store input */
		  [ptr]			"m" (*ptr),
		  [off]			"er" (off),
		  [inc]			"er" (inc)
		: "memory", "cc", "rax", "rbx", "rcx"
		  RSEQ_INJECT_CLOBBER
		: abort
#ifdef RSEQ_COMPARE_TWICE
		  , error1
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
#endif
}

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbne_load_cbne_store__ptr)(intptr_t *v, intptr_t expect,
			      intptr_t *v2, intptr_t expect2,
			      intptr_t newv, int cpu)
{
	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[ne])
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error2])
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error3])
#endif
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]))
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
		"cmpq %[v], %[expect]\n\t"
		"jne %l[ne]\n\t"
		RSEQ_INJECT_ASM(4)
		"cmpq %[v2], %[expect2]\n\t"
		"jne %l[ne]\n\t"
		RSEQ_INJECT_ASM(5)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), %l[error1])
		"cmpq %[v], %[expect]\n\t"
		"jne %l[error2]\n\t"
		"cmpq %[v2], %[expect2]\n\t"
		"jne %l[error3]\n\t"
#endif
		/* final store */
		"movq %[newv], %[v]\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(6)
		RSEQ_ASM_DEFINE_ABORT(4, "", abort)
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  /* cmp2 input */
		  [v2]			"m" (*v2),
		  [expect2]		"r" (expect2),
		  /* final store input */
		  [v]			"m" (*v),
		  [expect]		"r" (expect),
		  [newv]		"r" (newv)
		: "memory", "cc", "rax"
		  RSEQ_INJECT_CLOBBER
		: abort, ne
#ifdef RSEQ_COMPARE_TWICE
		  , error1, error2, error3
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
ne:
	rseq_after_asm_goto();
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_after_asm_goto();
	rseq_bug("1st expected value comparison failed");
error3:
	rseq_after_asm_goto();
	rseq_bug("2nd expected value comparison failed");
#endif
}

#endif /* #if defined(RSEQ_TEMPLATE_MO_RELAXED) &&
	(defined(RSEQ_TEMPLATE_INDEX_CPU_ID) || defined(RSEQ_TEMPLATE_INDEX_MM_CID)) */

#if (defined(RSEQ_TEMPLATE_MO_RELAXED) || defined(RSEQ_TEMPLATE_MO_RELEASE)) && \
	(defined(RSEQ_TEMPLATE_INDEX_CPU_ID) || defined(RSEQ_TEMPLATE_INDEX_MM_CID))

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbne_store_store__ptr)(intptr_t *v, intptr_t expect,
				 intptr_t *v2, intptr_t newv2,
				 intptr_t newv, int cpu)
{
	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[ne])
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error2])
#endif
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]))
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
		"cmpq %[v], %[expect]\n\t"
		"jne %l[ne]\n\t"
		RSEQ_INJECT_ASM(4)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), %l[error1])
		"cmpq %[v], %[expect]\n\t"
		"jne %l[error2]\n\t"
#endif
		/* try store */
		"movq %[newv2], %[v2]\n\t"
		RSEQ_INJECT_ASM(5)
		/* final store */
		"movq %[newv], %[v]\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(6)
		RSEQ_ASM_DEFINE_ABORT(4, "", abort)
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  /* try store input */
		  [v2]			"m" (*v2),
		  [newv2]		"r" (newv2),
		  /* final store input */
		  [v]			"m" (*v),
		  [expect]		"r" (expect),
		  [newv]		"r" (newv)
		: "memory", "cc", "rax"
		  RSEQ_INJECT_CLOBBER
		: abort, ne
#ifdef RSEQ_COMPARE_TWICE
		  , error1, error2
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
ne:
	rseq_after_asm_goto();
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_after_asm_goto();
	rseq_bug("expected value comparison failed");
#endif
}

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbne_memcpy_store__ptr)(intptr_t *v, intptr_t expect,
				 void *dst, void *src, size_t len,
				 intptr_t newv, int cpu)
{
	uint64_t rseq_scratch[3];

	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[ne])
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error2])
#endif
		"movq %[src], %[rseq_scratch0]\n\t"
		"movq %[dst], %[rseq_scratch1]\n\t"
		"movq %[len], %[rseq_scratch2]\n\t"
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]))
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
		"cmpq %[v], %[expect]\n\t"
		"jne 5f\n\t"
		RSEQ_INJECT_ASM(4)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 6f)
		"cmpq %[v], %[expect]\n\t"
		"jne 7f\n\t"
#endif
		/* try memcpy */
		"test %[len], %[len]\n\t" \
		"je 333f\n\t" \
		"222:\n\t" \
		"movb (%[src]), %%al\n\t" \
		"movb %%al, (%[dst])\n\t" \
		"inc %[src]\n\t" \
		"inc %[dst]\n\t" \
		"dec %[len]\n\t" \
		"jnz 222b\n\t" \
		"333:\n\t" \
		RSEQ_INJECT_ASM(5)
		/* final store */
		"movq %[newv], %[v]\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(6)
		/* teardown */
		"movq %[rseq_scratch2], %[len]\n\t"
		"movq %[rseq_scratch1], %[dst]\n\t"
		"movq %[rseq_scratch0], %[src]\n\t"
		RSEQ_ASM_DEFINE_ABORT(4,
			"movq %[rseq_scratch2], %[len]\n\t"
			"movq %[rseq_scratch1], %[dst]\n\t"
			"movq %[rseq_scratch0], %[src]\n\t",
			abort)
		RSEQ_ASM_DEFINE_TEARDOWN(5,
			"movq %[rseq_scratch2], %[len]\n\t"
			"movq %[rseq_scratch1], %[dst]\n\t"
			"movq %[rseq_scratch0], %[src]\n\t",
			ne)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_TEARDOWN(6,
			"movq %[rseq_scratch2], %[len]\n\t"
			"movq %[rseq_scratch1], %[dst]\n\t"
			"movq %[rseq_scratch0], %[src]\n\t",
			error1)
		RSEQ_ASM_DEFINE_TEARDOWN(7,
			"movq %[rseq_scratch2], %[len]\n\t"
			"movq %[rseq_scratch1], %[dst]\n\t"
			"movq %[rseq_scratch0], %[src]\n\t",
			error2)
#endif
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  /* final store input */
		  [v]			"m" (*v),
		  [expect]		"r" (expect),
		  [newv]		"r" (newv),
		  /* try memcpy input */
		  [dst]			"r" (dst),
		  [src]			"r" (src),
		  [len]			"r" (len),
		  [rseq_scratch0]	"m" (rseq_scratch[0]),
		  [rseq_scratch1]	"m" (rseq_scratch[1]),
		  [rseq_scratch2]	"m" (rseq_scratch[2])
		: "memory", "cc", "rax"
		  RSEQ_INJECT_CLOBBER
		: abort, ne
#ifdef RSEQ_COMPARE_TWICE
		  , error1, error2
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
ne:
	rseq_after_asm_goto();
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_after_asm_goto();
	rseq_bug("expected value comparison failed");
#endif
}

#endif /* #if (defined(RSEQ_TEMPLATE_MO_RELAXED) || defined(RSEQ_TEMPLATE_MO_RELEASE)) &&
	(defined(RSEQ_TEMPLATE_INDEX_CPU_ID) || defined(RSEQ_TEMPLATE_INDEX_MM_CID)) */

#elif defined(__i386__)

/*
 * On x86-32, use eax as scratch register and take memory operands as
 * input to lessen register pressure. Especially needed when compiling
 * in O0.
 */

#if defined(RSEQ_TEMPLATE_MO_RELAXED) && \
	(defined(RSEQ_TEMPLATE_INDEX_CPU_ID) || defined(RSEQ_TEMPLATE_INDEX_MM_CID))

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbne_store__ptr)(intptr_t *v, intptr_t expect, intptr_t newv, int cpu)
{
	/*
	 * ref_ip is used to store a reference instruction pointer
	 * for ip-relative addressing.
	 */
	struct rseq_local {
		uint32_t ref_ip;
	} rseq_local;

	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[ne])
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error2])
#endif
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]), %[ref_ip], RSEQ_ASM_REF_LABEL)
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
		"cmpl %[v], %[expect]\n\t"
		"jne %l[ne]\n\t"
		RSEQ_INJECT_ASM(4)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), %l[error1])
		"cmpl %[v], %[expect]\n\t"
		"jne %l[error2]\n\t"
#endif
		/* final store */
		"movl %[newv], %[v]\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(5)
		RSEQ_ASM_DEFINE_ABORT(4, "", abort)
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  [v]			"m" (*v),
		  [expect]		"r" (expect),
		  [newv]		"r" (newv),
		  [ref_ip]		"m" (rseq_local.ref_ip)
		: "memory", "cc", "eax"
		  RSEQ_INJECT_CLOBBER
		: abort, ne
#ifdef RSEQ_COMPARE_TWICE
		  , error1, error2
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
ne:
	rseq_after_asm_goto();
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_after_asm_goto();
	rseq_bug("expected value comparison failed");
#endif
}

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbeq_store_add_load_store__ptr)(intptr_t *v, intptr_t expectnot,
			       long voffp, intptr_t *load, int cpu)
{
	/*
	 * ref_ip is used to store a reference instruction pointer
	 * for ip-relative addressing.
	 */
	struct rseq_local {
		uint32_t ref_ip;
	} rseq_local;

	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[eq])
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error2])
#endif
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]), %[ref_ip], RSEQ_ASM_REF_LABEL)
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
		"movl %[v], %%ebx\n\t"
		"cmpl %%ebx, %[expectnot]\n\t"
		"je %l[eq]\n\t"
		RSEQ_INJECT_ASM(4)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), %l[error1])
		"movl %[v], %%ebx\n\t"
		"cmpl %%ebx, %[expectnot]\n\t"
		"je %l[error2]\n\t"
#endif
		"movl %%ebx, %[load]\n\t"
		"addl %[voffp], %%ebx\n\t"
		"movl (%%ebx), %%ebx\n\t"
		/* final store */
		"movl %%ebx, %[v]\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(5)
		RSEQ_ASM_DEFINE_ABORT(4, "", abort)
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  /* final store input */
		  [v]			"m" (*v),
		  [expectnot]		"r" (expectnot),
		  [voffp]		"ir" (voffp),
		  [load]		"m" (*load),
		  [ref_ip]		"m" (rseq_local.ref_ip)
		: "memory", "cc", "eax", "ebx"
		  RSEQ_INJECT_CLOBBER
		: abort, eq
#ifdef RSEQ_COMPARE_TWICE
		  , error1, error2
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
eq:
	rseq_after_asm_goto();
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_after_asm_goto();
	rseq_bug("expected value comparison failed");
#endif
}

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_add_store__ptr)(intptr_t *v, intptr_t count, int cpu)
{
	/*
	 * ref_ip is used to store a reference instruction pointer
	 * for ip-relative addressing.
	 */
	struct rseq_local {
		uint32_t ref_ip;
	} rseq_local;

	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
#endif
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]), %[ref_ip], RSEQ_ASM_REF_LABEL)
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), %l[error1])
#endif
		/* final store */
		"addl %[count], %[v]\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(4)
		RSEQ_ASM_DEFINE_ABORT(4, "", abort)
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  /* final store input */
		  [v]			"m" (*v),
		  [count]		"ir" (count),
		  [ref_ip]		"m" (rseq_local.ref_ip)
		: "memory", "cc", "eax"
		  RSEQ_INJECT_CLOBBER
		: abort
#ifdef RSEQ_COMPARE_TWICE
		  , error1
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
#endif
}

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbne_load_cbne_store__ptr)(intptr_t *v, intptr_t expect,
			      intptr_t *v2, intptr_t expect2,
			      intptr_t newv, int cpu)
{
	/*
	 * ref_ip is used to store a reference instruction pointer
	 * for ip-relative addressing.
	 */
	struct rseq_local {
		uint32_t ref_ip;
	} rseq_local;

	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[ne])
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error2])
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error3])
#endif
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]), %[ref_ip], RSEQ_ASM_REF_LABEL)
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
		"cmpl %[v], %[expect]\n\t"
		"jne %l[ne]\n\t"
		RSEQ_INJECT_ASM(4)
		"cmpl %[expect2], %[v2]\n\t"
		"jne %l[ne]\n\t"
		RSEQ_INJECT_ASM(5)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), %l[error1])
		"cmpl %[v], %[expect]\n\t"
		"jne %l[error2]\n\t"
		"cmpl %[expect2], %[v2]\n\t"
		"jne %l[error3]\n\t"
#endif
		"movl %[newv], %%eax\n\t"
		/* final store */
		"movl %%eax, %[v]\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(6)
		RSEQ_ASM_DEFINE_ABORT(4, "", abort)
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  /* cmp2 input */
		  [v2]			"m" (*v2),
		  [expect2]		"r" (expect2),
		  /* final store input */
		  [v]			"m" (*v),
		  [expect]		"r" (expect),
		  [newv]		"m" (newv),
		  [ref_ip]		"m" (rseq_local.ref_ip)
		: "memory", "cc", "eax"
		  RSEQ_INJECT_CLOBBER
		: abort, ne
#ifdef RSEQ_COMPARE_TWICE
		  , error1, error2, error3
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
ne:
	rseq_after_asm_goto();
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_after_asm_goto();
	rseq_bug("1st expected value comparison failed");
error3:
	rseq_after_asm_goto();
	rseq_bug("2nd expected value comparison failed");
#endif
}

#endif /* #if defined(RSEQ_TEMPLATE_MO_RELAXED) &&
	(defined(RSEQ_TEMPLATE_INDEX_CPU_ID) || defined(RSEQ_TEMPLATE_INDEX_MM_CID)) */

#if (defined(RSEQ_TEMPLATE_MO_RELAXED) || defined(RSEQ_TEMPLATE_MO_RELEASE)) && \
	(defined(RSEQ_TEMPLATE_INDEX_CPU_ID) || defined(RSEQ_TEMPLATE_INDEX_MM_CID))

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbne_store_store__ptr)(intptr_t *v, intptr_t expect,
					 intptr_t *v2, intptr_t newv2,
					 intptr_t newv, int cpu)
{
	/*
	 * ref_ip is used to store a reference instruction pointer
	 * for ip-relative addressing.
	 */
	struct rseq_local {
		uint32_t ref_ip;
	} rseq_local;

	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[ne])
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error2])
#endif
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]), %[ref_ip], RSEQ_ASM_REF_LABEL)
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
		"movl %[expect], %%eax\n\t"
		"cmpl %[v], %%eax\n\t"
		"jne %l[ne]\n\t"
		RSEQ_INJECT_ASM(4)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), %l[error1])
		"movl %[expect], %%eax\n\t"
		"cmpl %[v], %%eax\n\t"
		"jne %l[error2]\n\t"
#endif
		/* try store */
		"movl %[newv2], %[v2]\n\t"
		RSEQ_INJECT_ASM(5)
#ifdef RSEQ_TEMPLATE_MO_RELEASE
		"lock; addl $0,-128(%%esp)\n\t"
#endif
		/* final store */
		"movl %[newv], %[v]\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(6)
		RSEQ_ASM_DEFINE_ABORT(4, "", abort)
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  /* try store input */
		  [v2]			"m" (*v2),
		  [newv2]		"r" (newv2),
		  /* final store input */
		  [v]			"m" (*v),
		  [expect]		"m" (expect),
		  [newv]		"r" (newv),
		  [ref_ip]		"m" (rseq_local.ref_ip)
		: "memory", "cc", "eax"
		  RSEQ_INJECT_CLOBBER
		: abort, ne
#ifdef RSEQ_COMPARE_TWICE
		  , error1, error2
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
ne:
	rseq_after_asm_goto();
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_after_asm_goto();
	rseq_bug("expected value comparison failed");
#endif

}

/* TODO: implement a faster memcpy. */
static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbne_memcpy_store__ptr)(intptr_t *v, intptr_t expect,
					 void *dst, void *src, size_t len,
					 intptr_t newv, int cpu)
{
	/*
	 * Work-around register pressure limitations.
	 * Old gcc does not support output operands for asm goto, so
	 * input registers cannot simply be re-used as output registers.
	 * This is why clobbered registers are used.
	 * ref_ip is used to store a reference instruction pointer
	 * for ip-relative addressing.
	 */
	struct rseq_local {
		uint32_t expect, dst, src, len, newv, ref_ip;
	} rseq_local = {
		.expect = (uint32_t) expect,
		.dst = (uint32_t) dst,
		.src = (uint32_t) src,
		.len = (uint32_t) len,
		.newv = (uint32_t) newv,
		.ref_ip = 0,
	};

	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto (
		RSEQ_ASM_DEFINE_TABLE(3, 1f, 2f, 4f) /* start, commit, abort */
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[ne])
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error1])
		RSEQ_ASM_DEFINE_EXIT_POINT(1f, %l[error2])
#endif
		/* Start rseq by storing table entry pointer into rseq_cs. */
		RSEQ_ASM_STORE_RSEQ_CS(1, 3b, RSEQ_ASM_TP_SEGMENT:RSEQ_ASM_CS_OFFSET(%[rseq_offset]), %[ref_ip], RSEQ_ASM_REF_LABEL)
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), 4f)
		RSEQ_INJECT_ASM(3)
		/* load expect into ebx */
		"movl %[expect], %%ebx\n\t"
		"cmpl %%ebx, %[v]\n\t"
		"jne %l[ne]\n\t"
		RSEQ_INJECT_ASM(4)
#ifdef RSEQ_COMPARE_TWICE
		RSEQ_ASM_CBNE_CPU_ID(cpu_id, RSEQ_ASM_TP_SEGMENT:RSEQ_TEMPLATE_INDEX_CPU_ID_OFFSET(%[rseq_offset]), %l[error1])
		"cmpl %%ebx, %[v]\n\t"
		"jne %l[error2]\n\t"
#endif
		/* try memcpy */
		/* load dst into ebx */
		"movl %[dst], %%ebx\n\t"
		/* load src into ecx */
		"movl %[src], %%ecx\n\t"
		/* load len into edx */
		"movl %[len], %%edx\n\t"
		"test %%edx, %%edx\n\t"
		"je 333f\n\t"
		"222:\n\t"
		"movb (%%ecx), %%al\n\t"
		"movb %%al, (%%ebx)\n\t"
		"inc %%ecx\n\t"
		"inc %%ebx\n\t"
		"dec %%edx\n\t"
		"jnz 222b\n\t"
		"333:\n\t"
		RSEQ_INJECT_ASM(5)
#ifdef RSEQ_TEMPLATE_MO_RELEASE
		"lock; addl $0,-128(%%esp)\n\t"
#endif
		/* load newv into ebx */
		"movl %[newv], %%ebx\n\t"
		/* final store */
		"movl %%ebx, %[v]\n\t"
		"2:\n\t"
		RSEQ_INJECT_ASM(6)
		RSEQ_ASM_DEFINE_ABORT(4, "", abort)
		: /* gcc asm goto does not allow outputs */
		: [cpu_id]		"r" (cpu),
		  [rseq_offset]		"r" (rseq_offset),
		  /* final store input */
		  [v]			"m" (*v),
		  /* try memcpy input */
		  [expect]		"m" (rseq_local.expect),	/* ebx */
		  [dst]			"m" (rseq_local.dst),		/* ebx */
		  [src]			"m" (rseq_local.src),		/* ecx */
		  [len]			"m" (rseq_local.len),		/* edx */
		  [newv]		"m" (rseq_local.newv),		/* ebx */
		  [ref_ip]		"m" (rseq_local.ref_ip)
		: "memory", "cc", "eax", "ebx", "ecx", "edx"
		  RSEQ_INJECT_CLOBBER
		: abort, ne
#ifdef RSEQ_COMPARE_TWICE
		  , error1, error2
#endif
	);
	rseq_after_asm_goto();
	return 0;
abort:
	rseq_after_asm_goto();
	RSEQ_INJECT_FAILED
	return -1;
ne:
	rseq_after_asm_goto();
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_after_asm_goto();
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_after_asm_goto();
	rseq_bug("expected value comparison failed");
#endif
}

#endif /* #if (defined(RSEQ_TEMPLATE_MO_RELAXED) || defined(RSEQ_TEMPLATE_MO_RELEASE)) &&
	(defined(RSEQ_TEMPLATE_INDEX_CPU_ID) || defined(RSEQ_TEMPLATE_INDEX_MM_CID)) */

#endif

#include "rseq/arch/templates/bits-reset.h"
