/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2024 Michael Jeanson <mjeanson@efficios.com> */

/*
 * rseq/arch/riscv/thread-pointer.h
 */

#ifndef _RSEQ_RISCV_THREAD_POINTER
#define _RSEQ_RISCV_THREAD_POINTER

#include <features.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __GNUC_PREREQ (10, 3)
static inline __attribute__((always_inline))
void *rseq_thread_pointer(void)
{
	return __builtin_thread_pointer();
}
#else
static inline __attribute__((always_inline))
void *rseq_thread_pointer(void)
{
	void *__result;

	__asm__ ("mv %0, tp" : "=r" (__result));
	return __result;
}
#endif /* !GCC 10.3 */

#ifdef __cplusplus
}
#endif

#endif
