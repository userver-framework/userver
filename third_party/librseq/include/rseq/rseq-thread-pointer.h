/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2021 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq-thread-pointer.h
 */

#ifndef _RSEQ_THREAD_POINTER
#define _RSEQ_THREAD_POINTER

#if defined(__x86_64__) || defined(__i386__)
#include <rseq/rseq-x86-thread-pointer.h>
#elif defined(__PPC__)
#include <rseq/rseq-ppc-thread-pointer.h>
#else
#include <rseq/rseq-generic-thread-pointer.h>
#endif

#endif
