/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2021 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq/thread-pointer.h
 */

#ifndef _RSEQ_THREAD_POINTER_H
#define _RSEQ_THREAD_POINTER_H

#if (defined(__amd64__) \
	|| defined(__amd64) \
	|| defined(__x86_64__) \
	|| defined(__x86_64) \
	|| defined(__i386__) \
	|| defined(__i386))

#include <rseq/arch/x86/thread-pointer.h>

#elif (defined(__powerpc64__) \
	|| defined(__ppc64__) \
	|| defined(__powerpc__) \
	|| defined(__powerpc) \
	|| defined(__ppc__))

#include <rseq/arch/ppc/thread-pointer.h>

#elif defined(__riscv)

#include <rseq/arch/riscv/thread-pointer.h>

#else

#include <rseq/arch/generic/thread-pointer.h>

#endif

#endif /* _RSEQ_THREAD_POINTER_H */
