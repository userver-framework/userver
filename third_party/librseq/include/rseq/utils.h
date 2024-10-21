/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2016-2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq/utils.h
 */

#ifndef _RSEQ_UTILS_H
#define _RSEQ_UTILS_H

#include <stddef.h>
#include <stdio.h>

#ifndef rseq_sizeof_field
#define rseq_sizeof_field(TYPE, MEMBER) sizeof((((TYPE *)0)->MEMBER))
#endif

#ifndef rseq_offsetofend
#define rseq_offsetofend(TYPE, MEMBER) \
	(offsetof(TYPE, MEMBER)	+ rseq_sizeof_field(TYPE, MEMBER))
#endif

#define __rseq_str_1(x)	#x
#define __rseq_str(x)		__rseq_str_1(x)

#define rseq_log(fmt, ...)						       \
	fprintf(stderr, fmt "(in %s() at " __FILE__ ":" __rseq_str(__LINE__)"\n", \
		## __VA_ARGS__, __func__)

#define rseq_bug(fmt, ...)		\
	do {				\
		rseq_log(fmt, ## __VA_ARGS__); \
		abort();		\
	} while (0)

#endif /* _RSEQ_UTILS_H */
