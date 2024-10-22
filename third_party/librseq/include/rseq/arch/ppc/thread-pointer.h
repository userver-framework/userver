/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2021 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq/arch/ppc/thread-pointer.h
 */

#ifndef _RSEQ_PPC_THREAD_POINTER
#define _RSEQ_PPC_THREAD_POINTER

#ifdef __cplusplus
extern "C" {
#endif

static inline __attribute__((always_inline))
void *rseq_thread_pointer(void)
{
#ifdef __powerpc64__
	register void *__result asm ("r13");
#else
	register void *__result asm ("r2");
#endif
	asm ("" : "=r" (__result));
	return __result;
}

#ifdef __cplusplus
}
#endif

#endif
