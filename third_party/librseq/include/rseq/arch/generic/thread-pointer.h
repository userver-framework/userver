/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2021 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq/arch/generic/thread-pointer.h
 */

#ifndef _RSEQ_GENERIC_THREAD_POINTER_H
#define _RSEQ_GENERIC_THREAD_POINTER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Use gcc builtin thread pointer. */
static inline __attribute__((always_inline))
void *rseq_thread_pointer(void)
{
	return __builtin_thread_pointer();
}

#ifdef __cplusplus
}
#endif

#endif
