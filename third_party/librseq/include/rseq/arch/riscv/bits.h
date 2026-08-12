/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2022 Vincent Chen <vincent.chen@sifive.com> */

#include "rseq/arch/templates/bits.h"

/*
 * Refer to rseq/pseudocode.h for documentation and pseudo-code of the
 * rseq critical section helpers.
 */
#include "rseq/pseudocode.h"

#if defined(RSEQ_TEMPLATE_MO_RELAXED) && \
	(defined(RSEQ_TEMPLATE_INDEX_CPU_ID) || defined(RSEQ_TEMPLATE_INDEX_MM_CID))

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbne_store__ptr)(intptr_t *v, intptr_t expect, intptr_t newv, int cpu)
{
	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto(RSEQ_ASM_DEFINE_TABLE(1, 2f, 3f, 4f)
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[ne]")
#ifdef RSEQ_COMPARE_TWICE
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[error1]")
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[error2]")
#endif
				  RSEQ_ASM_STORE_RSEQ_CS(2, 1b, rseq_cs)
				  RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, 4f)
				  RSEQ_INJECT_ASM(3)
				  RSEQ_ASM_OP_CBNE(v, expect, "%l[ne]")
				  RSEQ_INJECT_ASM(4)
#ifdef RSEQ_COMPARE_TWICE
				  RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, "%l[error1]")
				  RSEQ_ASM_OP_CBNE(v, expect, "%l[error2]")
#endif
				  RSEQ_ASM_OP_FINAL_STORE(newv, v, 3)
				  RSEQ_INJECT_ASM(5)
				  RSEQ_ASM_DEFINE_ABORT(4, "", abort)
				  : /* gcc asm goto does not allow outputs */
				  : [cpu_id]		"r" (cpu),
				    [current_cpu_id]	"m" (rseq_get_abi()->RSEQ_TEMPLATE_INDEX_CPU_ID_FIELD),
				    [rseq_cs]		"m" (rseq_get_abi()->rseq_cs.arch.ptr),
				    [v]			"m" (*v),
				    [expect]		"r" (expect),
				    [newv]		"r" (newv)
				    RSEQ_INJECT_INPUT
				  : "memory", RSEQ_ASM_TMP_REG_1
				    RSEQ_INJECT_CLOBBER
				  : abort, ne
#ifdef RSEQ_COMPARE_TWICE
				    , error1, error2
#endif
	);

	return 0;
abort:
	RSEQ_INJECT_FAILED
	return -1;
ne:
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_bug("expected value comparison failed");
#endif
}

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbeq_store_add_load_store__ptr)(intptr_t *v, intptr_t expectnot,
			       off_t voffp, intptr_t *load, int cpu)
{
	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto(RSEQ_ASM_DEFINE_TABLE(1, 2f, 3f, 4f)
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[eq]")
#ifdef RSEQ_COMPARE_TWICE
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[error1]")
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[error2]")
#endif
				  RSEQ_ASM_STORE_RSEQ_CS(2, 1b, rseq_cs)
				  RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, 4f)
				  RSEQ_INJECT_ASM(3)
				  RSEQ_ASM_OP_CBEQ(v, expectnot, "%l[eq]")
				  RSEQ_INJECT_ASM(4)
#ifdef RSEQ_COMPARE_TWICE
				  RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, "%l[error1]")
				  RSEQ_ASM_OP_CBEQ(v, expectnot, "%l[error2]")
#endif
				  RSEQ_ASM_OP_R_LOAD(v)
				  RSEQ_ASM_OP_R_STORE(load)
				  RSEQ_ASM_OP_R_LOAD_OFF(voffp)
				  RSEQ_ASM_OP_R_FINAL_STORE(v, 3)
				  RSEQ_INJECT_ASM(5)
				  RSEQ_ASM_DEFINE_ABORT(4, "", abort)
				  : /* gcc asm goto does not allow outputs */
				  : [cpu_id]		"r" (cpu),
				    [current_cpu_id]	"m" (rseq_get_abi()->RSEQ_TEMPLATE_INDEX_CPU_ID_FIELD),
				    [rseq_cs]		"m" (rseq_get_abi()->rseq_cs.arch.ptr),
				    [v]			"m" (*v),
				    [expectnot]		"r" (expectnot),
				    [load]		"m" (*load),
				    [voffp]		"r" (voffp)
				    RSEQ_INJECT_INPUT
				  : "memory", RSEQ_ASM_TMP_REG_1
				    RSEQ_INJECT_CLOBBER
				  : abort, eq
#ifdef RSEQ_COMPARE_TWICE
				    , error1, error2
#endif
	);
	return 0;
abort:
	RSEQ_INJECT_FAILED
	return -1;
eq:
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_bug("expected value comparison failed");
#endif
}

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_add_store__ptr)(intptr_t *v, intptr_t count, int cpu)
{
	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto(RSEQ_ASM_DEFINE_TABLE(1, 2f, 3f, 4f)
#ifdef RSEQ_COMPARE_TWICE
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[error1]")
#endif
				  RSEQ_ASM_STORE_RSEQ_CS(2, 1b, rseq_cs)
				  RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, 4f)
				  RSEQ_INJECT_ASM(3)
#ifdef RSEQ_COMPARE_TWICE
				  RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, "%l[error1]")
#endif
				  RSEQ_ASM_OP_R_LOAD(v)
				  RSEQ_ASM_OP_R_ADD(count)
				  RSEQ_ASM_OP_R_FINAL_STORE(v, 3)
				  RSEQ_INJECT_ASM(4)
				  RSEQ_ASM_DEFINE_ABORT(4, "", abort)
				  : /* gcc asm goto does not allow outputs */
				  : [cpu_id]		"r" (cpu),
				    [current_cpu_id]	"m" (rseq_get_abi()->RSEQ_TEMPLATE_INDEX_CPU_ID_FIELD),
				    [rseq_cs]		"m" (rseq_get_abi()->rseq_cs.arch.ptr),
				    [v]			"m" (*v),
				    [count]		"r" (count)
				    RSEQ_INJECT_INPUT
				  : "memory", RSEQ_ASM_TMP_REG_1
				    RSEQ_INJECT_CLOBBER
				  : abort
#ifdef RSEQ_COMPARE_TWICE
				    , error1
#endif
	);
	return 0;
abort:
	RSEQ_INJECT_FAILED
	return -1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_bug("cpu_id comparison failed");
#endif
}

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbne_load_cbne_store__ptr)(intptr_t *v, intptr_t expect,
			      intptr_t *v2, intptr_t expect2,
			      intptr_t newv, int cpu)
{
	RSEQ_INJECT_C(9)

	__asm__ __volatile__ goto(RSEQ_ASM_DEFINE_TABLE(1, 2f, 3f, 4f)
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[ne]")
#ifdef RSEQ_COMPARE_TWICE
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[error1]")
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[error2]")
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[error3]")
#endif
				  RSEQ_ASM_STORE_RSEQ_CS(2, 1b, rseq_cs)
				  RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, 4f)
				  RSEQ_INJECT_ASM(3)
				  RSEQ_ASM_OP_CBNE(v, expect, "%l[ne]")
				  RSEQ_INJECT_ASM(4)
				  RSEQ_ASM_OP_CBNE(v2, expect2, "%l[ne]")
				  RSEQ_INJECT_ASM(5)
#ifdef RSEQ_COMPARE_TWICE
				  RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, "%l[error1]")
				  RSEQ_ASM_OP_CBNE(v, expect, "%l[error2]")
				  RSEQ_ASM_OP_CBNE(v2, expect2, "%l[error3]")
#endif
				  RSEQ_ASM_OP_FINAL_STORE(newv, v, 3)
				  RSEQ_INJECT_ASM(6)
				  RSEQ_ASM_DEFINE_ABORT(4, "", abort)
				  : /* gcc asm goto does not allow outputs */
				  : [cpu_id]		"r" (cpu),
				    [current_cpu_id]	"m" (rseq_get_abi()->RSEQ_TEMPLATE_INDEX_CPU_ID_FIELD),
				    [rseq_cs]		"m" (rseq_get_abi()->rseq_cs.arch.ptr),
				    [v]			"m" (*v),
				    [expect]		"r" (expect),
				    [v2]			"m" (*v2),
				    [expect2]		"r" (expect2),
				    [newv]		"r" (newv)
				    RSEQ_INJECT_INPUT
				  : "memory", RSEQ_ASM_TMP_REG_1
				    RSEQ_INJECT_CLOBBER
				  : abort, ne
#ifdef RSEQ_COMPARE_TWICE
				    , error1, error2, error3
#endif
	);

	return 0;
abort:
	RSEQ_INJECT_FAILED
	return -1;
ne:
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_bug("expected value comparison failed");
error3:
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

	__asm__ __volatile__ goto(RSEQ_ASM_DEFINE_TABLE(1, 2f, 3f, 4f)
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[ne]")
#ifdef RSEQ_COMPARE_TWICE
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[error1]")
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[error2]")
#endif
				  RSEQ_ASM_STORE_RSEQ_CS(2, 1b, rseq_cs)
				  RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, 4f)
				  RSEQ_INJECT_ASM(3)
				  RSEQ_ASM_OP_CBNE(v, expect, "%l[ne]")
				  RSEQ_INJECT_ASM(4)
#ifdef RSEQ_COMPARE_TWICE
				  RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, "%l[error1]")
				  RSEQ_ASM_OP_CBNE(v, expect, "%l[error2]")
#endif
				  RSEQ_ASM_OP_STORE(newv2, v2)
				  RSEQ_INJECT_ASM(5)
#ifdef RSEQ_TEMPLATE_MO_RELEASE
				  RSEQ_ASM_OP_FINAL_STORE_RELEASE(newv, v, 3)
#else
				  RSEQ_ASM_OP_FINAL_STORE(newv, v, 3)
#endif
				  RSEQ_INJECT_ASM(6)
				  RSEQ_ASM_DEFINE_ABORT(4, "", abort)
				  : /* gcc asm goto does not allow outputs */
				  : [cpu_id]			"r" (cpu),
				    [current_cpu_id]		"m" (rseq_get_abi()->RSEQ_TEMPLATE_INDEX_CPU_ID_FIELD),
				    [rseq_cs]			"m" (rseq_get_abi()->rseq_cs.arch.ptr),
				    [expect]			"r" (expect),
				    [v]				"m" (*v),
				    [newv]			"r" (newv),
				    [v2]			"m" (*v2),
				    [newv2]			"r" (newv2)
				    RSEQ_INJECT_INPUT
				  : "memory", RSEQ_ASM_TMP_REG_1
				    RSEQ_INJECT_CLOBBER
				  : abort, ne
#ifdef RSEQ_COMPARE_TWICE
				    , error1, error2
#endif
	);

	return 0;
abort:
	RSEQ_INJECT_FAILED
	return -1;
ne:
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_bug("expected value comparison failed");
#endif
}

static inline __attribute__((always_inline))
int RSEQ_TEMPLATE_IDENTIFIER(rseq_load_cbne_memcpy_store__ptr)(intptr_t *v, intptr_t expect,
				 void *dst, void *src, size_t len,
				 intptr_t newv, int cpu)
{
	RSEQ_INJECT_C(9)
	__asm__ __volatile__ goto(RSEQ_ASM_DEFINE_TABLE(1, 2f, 3f, 4f)
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[ne]")
#ifdef RSEQ_COMPARE_TWICE
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[error1]")
				  RSEQ_ASM_DEFINE_EXIT_POINT(2f, "%l[error2]")
#endif
				  RSEQ_ASM_STORE_RSEQ_CS(2, 1b, rseq_cs)
				  RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, 4f)
				  RSEQ_INJECT_ASM(3)
				  RSEQ_ASM_OP_CBNE(v, expect, "%l[ne]")
				  RSEQ_INJECT_ASM(4)
#ifdef RSEQ_COMPARE_TWICE
				  RSEQ_ASM_CBNE_CPU_ID(cpu_id, current_cpu_id, "%l[error1]")
				  RSEQ_ASM_OP_CBNE(v, expect, "%l[error2]")
#endif
				  RSEQ_ASM_OP_R_BYTEWISE_MEMCPY(dst, src, len)
				  RSEQ_INJECT_ASM(5)
#ifdef RSEQ_TEMPLATE_MO_RELEASE
				  RSEQ_ASM_OP_FINAL_STORE_RELEASE(newv, v, 3)
#else
				  RSEQ_ASM_OP_FINAL_STORE(newv, v, 3)
#endif
				  RSEQ_INJECT_ASM(6)
				  RSEQ_ASM_DEFINE_ABORT(4, "", abort)
				  : /* gcc asm goto does not allow outputs */
				  : [cpu_id]			"r" (cpu),
				    [current_cpu_id]		"m" (rseq_get_abi()->RSEQ_TEMPLATE_INDEX_CPU_ID_FIELD),
				    [rseq_cs]			"m" (rseq_get_abi()->rseq_cs.arch.ptr),
				    [expect]			"r" (expect),
				    [v]				"m" (*v),
				    [newv]			"r" (newv),
				    [dst]			"r" (dst),
				    [src]			"r" (src),
				    [len]			"r" (len)
				    RSEQ_INJECT_INPUT
				  : "memory", RSEQ_ASM_TMP_REG_1, RSEQ_ASM_TMP_REG_2,
				    RSEQ_ASM_TMP_REG_3, RSEQ_ASM_TMP_REG_4
				    RSEQ_INJECT_CLOBBER
				  : abort, ne
#ifdef RSEQ_COMPARE_TWICE
				    , error1, error2
#endif
	);

	return 0;
abort:
	RSEQ_INJECT_FAILED
	return -1;
ne:
	return 1;
#ifdef RSEQ_COMPARE_TWICE
error1:
	rseq_bug("cpu_id comparison failed");
error2:
	rseq_bug("expected value comparison failed");
#endif
}

#endif /* #if (defined(RSEQ_TEMPLATE_MO_RELAXED) || defined(RSEQ_TEMPLATE_MO_RELEASE)) &&
	(defined(RSEQ_TEMPLATE_INDEX_CPU_ID) || defined(RSEQ_TEMPLATE_INDEX_MM_CID)) */

#include "rseq/arch/templates/bits-reset.h"
