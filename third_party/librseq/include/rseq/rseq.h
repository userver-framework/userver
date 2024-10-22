/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2016-2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq/rseq.h
 */

#ifndef _RSEQ_RSEQ_H
#define _RSEQ_RSEQ_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <sched.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

#include <rseq/abi.h>
#include <rseq/compiler.h>
#include <rseq/inject.h>
#include <rseq/thread-pointer.h>
#include <rseq/utils.h>

enum rseq_mo {
	RSEQ_MO_RELAXED = 0,
	RSEQ_MO_CONSUME = 1,	/* Unused */
	RSEQ_MO_ACQUIRE = 2,	/* Unused */
	RSEQ_MO_RELEASE = 3,
	RSEQ_MO_ACQ_REL = 4,	/* Unused */
	RSEQ_MO_SEQ_CST = 5,	/* Unused */
};

enum rseq_percpu_mode {
	RSEQ_PERCPU_CPU_ID = 0,
	RSEQ_PERCPU_MM_CID = 1,
};

enum rseq_available_query {
	RSEQ_AVAILABLE_QUERY_KERNEL = 0,
	RSEQ_AVAILABLE_QUERY_LIBC = 1,
};

/*
 * User code can define RSEQ_GET_ABI_OVERRIDE to override the
 * rseq_get_abi() implementation, for instance to use glibc's symbols
 * directly.
 */
#ifndef RSEQ_GET_ABI_OVERRIDE

# ifdef __cplusplus
extern "C" {
# endif

/* Offset from the thread pointer to the rseq area. */
extern ptrdiff_t rseq_offset;

/*
 * The rseq ABI is composed of extensible feature fields. The extensions
 * are done by appending additional fields at the end of the structure.
 * The rseq_size defines the size of the active feature set which can be
 * used by the application for the current rseq registration. Features
 * starting at offset >= rseq_size are inactive and should not be used.
 *
 * The rseq_size is the intersection between the available allocation
 * size for the rseq area and the feature size supported by the kernel.
 */

/*
 * Size of the active rseq feature set. 0 if the registration was
 * unsuccessful.
 */
extern unsigned int rseq_size;

/* Flags used during rseq registration. */
extern unsigned int rseq_flags;

/*
 * Returns a pointer to the rseq area.
 */
static inline __attribute__((always_inline))
struct rseq_abi *rseq_get_abi(void)
{
	return (struct rseq_abi *) ((uintptr_t) rseq_thread_pointer() + rseq_offset);
}

# ifdef __cplusplus
}
# endif

#endif /* RSEQ_GET_ABI_OVERRIDE */


/*
 * Architecture specific.
 */
#include <rseq/arch.h>


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Register rseq for the current thread. This needs to be called once
 * by any thread which uses restartable sequences, before they start
 * using restartable sequences, to ensure restartable sequences
 * succeed. A restartable sequence executed from a non-registered
 * thread will always fail.
 */
int rseq_register_current_thread(void);

/*
 * Unregister rseq for current thread.
 */
int rseq_unregister_current_thread(void);

/*
 * Restartable sequence fallback for reading the current CPU number.
 */
int32_t rseq_fallback_current_cpu(void);

/*
 * Restartable sequence fallback for reading the current node number.
 */
int32_t rseq_fallback_current_node(void);

/*
 * Returns true if rseq is supported.
 */
bool rseq_available(unsigned int query);

/*
 * rseq_get_max_nr_cpus: Get the max_nr_cpus auto-detected value.
 *
 * Returns the max_nr_cpus auto-detected at pool creation when invoked
 * with @nr_max_cpus=0 argument.
 */
int rseq_get_max_nr_cpus(void);

/*
 * Values returned can be either the current CPU number, -1 (rseq is
 * uninitialized), or -2 (rseq initialization has failed).
 */
static inline __attribute__((always_inline))
int32_t rseq_current_cpu_raw(void)
{
	return RSEQ_READ_ONCE(rseq_get_abi()->cpu_id);
}

/*
 * Returns a possible CPU number, which is typically the current CPU.
 * The returned CPU number can be used to prepare for an rseq critical
 * section, which will confirm whether the cpu number is indeed the
 * current one, and whether rseq is initialized.
 *
 * The CPU number returned by rseq_cpu_start should always be validated
 * by passing it to a rseq asm sequence, or by comparing it to the
 * return value of rseq_current_cpu_raw() if the rseq asm sequence
 * does not need to be invoked.
 */
static inline __attribute__((always_inline))
uint32_t rseq_cpu_start(void)
{
	return RSEQ_READ_ONCE(rseq_get_abi()->cpu_id_start);
}

static inline __attribute__((always_inline))
uint32_t rseq_current_cpu(void)
{
	int32_t cpu;

	cpu = rseq_current_cpu_raw();
	if (rseq_unlikely(cpu < 0))
		cpu = rseq_fallback_current_cpu();
	return cpu;
}

static inline __attribute__((always_inline))
bool rseq_node_id_available(void)
{
	return (int) rseq_size >= (int) rseq_offsetofend(struct rseq_abi, node_id);
}

/*
 * Current NUMA node number.
 */
static inline __attribute__((always_inline))
uint32_t rseq_current_node_id(void)
{
	assert(rseq_node_id_available());
	return RSEQ_READ_ONCE(rseq_get_abi()->node_id);
}

static inline __attribute__((always_inline))
bool rseq_mm_cid_available(void)
{
	return (int) rseq_size >= (int) rseq_offsetofend(struct rseq_abi, mm_cid);
}

static inline __attribute__((always_inline))
uint32_t rseq_current_mm_cid(void)
{
	return RSEQ_READ_ONCE(rseq_get_abi()->mm_cid);
}

static inline __attribute__((always_inline))
void rseq_clear_rseq_cs(void)
{
	RSEQ_WRITE_ONCE(rseq_get_abi()->rseq_cs.arch.ptr, 0);
}

/*
 * rseq_prepare_unload() should be invoked by each thread executing a rseq
 * critical section at least once between their last critical section and
 * library unload of the library defining the rseq critical section (struct
 * rseq_cs) or the code referred to by the struct rseq_cs start_ip and
 * post_commit_offset fields. This also applies to use of rseq in code
 * generated by JIT: rseq_prepare_unload() should be invoked at least once by
 * each thread executing a rseq critical section before reclaim of the memory
 * holding the struct rseq_cs or reclaim of the code pointed to by struct
 * rseq_cs start_ip and post_commit_offset fields.
 */
static inline __attribute__((always_inline))
void rseq_prepare_unload(void)
{
	rseq_clear_rseq_cs();
}

/*
 * Refer to rseq/pseudocode.h for documentation and pseudo-code of the
 * rseq critical section helpers.
 */
#include "rseq/pseudocode.h"

static inline __attribute__((always_inline))
int rseq_load_cbne_store__ptr(enum rseq_mo rseq_mo, enum rseq_percpu_mode percpu_mode,
		       intptr_t *v, intptr_t expect,
		       intptr_t newv, int cpu)
{
	if (rseq_mo != RSEQ_MO_RELAXED)
		return -1;
	switch (percpu_mode) {
	case RSEQ_PERCPU_CPU_ID:
		return rseq_load_cbne_store__ptr_relaxed_cpu_id(v, expect, newv, cpu);
	case RSEQ_PERCPU_MM_CID:
		return rseq_load_cbne_store__ptr_relaxed_mm_cid(v, expect, newv, cpu);
	default:
		return -1;
	}
}

static inline __attribute__((always_inline))
int rseq_load_cbeq_store_add_load_store__ptr(enum rseq_mo rseq_mo, enum rseq_percpu_mode percpu_mode,
			       intptr_t *v, intptr_t expectnot, long voffp, intptr_t *load,
			       int cpu)
{
	if (rseq_mo != RSEQ_MO_RELAXED)
		return -1;
	switch (percpu_mode) {
	case RSEQ_PERCPU_CPU_ID:
		return rseq_load_cbeq_store_add_load_store__ptr_relaxed_cpu_id(v, expectnot, voffp, load, cpu);
	case RSEQ_PERCPU_MM_CID:
		return rseq_load_cbeq_store_add_load_store__ptr_relaxed_mm_cid(v, expectnot, voffp, load, cpu);
	default:
		return -1;
	}
}

static inline __attribute__((always_inline))
int rseq_load_add_store__ptr(enum rseq_mo rseq_mo, enum rseq_percpu_mode percpu_mode,
	      intptr_t *v, intptr_t count, int cpu)
{
	if (rseq_mo != RSEQ_MO_RELAXED)
		return -1;
	switch (percpu_mode) {
	case RSEQ_PERCPU_CPU_ID:
		return rseq_load_add_store__ptr_relaxed_cpu_id(v, count, cpu);
	case RSEQ_PERCPU_MM_CID:
		return rseq_load_add_store__ptr_relaxed_mm_cid(v, count, cpu);
	default:
		return -1;
	}
}

#ifdef rseq_arch_has_load_add_load_load_add_store
static inline __attribute__((always_inline))
int rseq_load_add_load_load_add_store__ptr(enum rseq_mo rseq_mo, enum rseq_percpu_mode percpu_mode,
			   intptr_t *ptr, long off, intptr_t inc, int cpu)
{
	if (rseq_mo != RSEQ_MO_RELAXED)
		return -1;
	switch (percpu_mode) {
	case RSEQ_PERCPU_CPU_ID:
		return rseq_load_add_load_load_add_store__ptr_relaxed_cpu_id(ptr, off, inc, cpu);
	case RSEQ_PERCPU_MM_CID:
		return rseq_load_add_load_load_add_store__ptr_relaxed_mm_cid(ptr, off, inc, cpu);
	default:
		return -1;
	}
}
#endif

static inline __attribute__((always_inline))
int rseq_load_cbne_store_store__ptr(enum rseq_mo rseq_mo, enum rseq_percpu_mode percpu_mode,
				 intptr_t *v, intptr_t expect,
				 intptr_t *v2, intptr_t newv2,
				 intptr_t newv, int cpu)
{
	switch (rseq_mo) {
	case RSEQ_MO_RELAXED:
		switch (percpu_mode) {
		case RSEQ_PERCPU_CPU_ID:
			return rseq_load_cbne_store_store__ptr_relaxed_cpu_id(v, expect, v2, newv2, newv, cpu);
		case RSEQ_PERCPU_MM_CID:
			return rseq_load_cbne_store_store__ptr_relaxed_mm_cid(v, expect, v2, newv2, newv, cpu);
		default:
			return -1;
		}
	case RSEQ_MO_RELEASE:
		switch (percpu_mode) {
		case RSEQ_PERCPU_CPU_ID:
			return rseq_load_cbne_store_store__ptr_release_cpu_id(v, expect, v2, newv2, newv, cpu);
		case RSEQ_PERCPU_MM_CID:
			return rseq_load_cbne_store_store__ptr_release_mm_cid(v, expect, v2, newv2, newv, cpu);
		default:
			return -1;
		}
	case RSEQ_MO_ACQUIRE:	/* Fallthrough */
	case RSEQ_MO_ACQ_REL:	/* Fallthrough */
	case RSEQ_MO_CONSUME:	/* Fallthrough */
	case RSEQ_MO_SEQ_CST:	/* Fallthrough */
	default:
		return -1;
	}
}

static inline __attribute__((always_inline))
int rseq_load_cbne_load_cbne_store__ptr(enum rseq_mo rseq_mo, enum rseq_percpu_mode percpu_mode,
			      intptr_t *v, intptr_t expect,
			      intptr_t *v2, intptr_t expect2,
			      intptr_t newv, int cpu)
{
	if (rseq_mo != RSEQ_MO_RELAXED)
		return -1;
	switch (percpu_mode) {
	case RSEQ_PERCPU_CPU_ID:
		return rseq_load_cbne_load_cbne_store__ptr_relaxed_cpu_id(v, expect, v2, expect2, newv, cpu);
	case RSEQ_PERCPU_MM_CID:
		return rseq_load_cbne_load_cbne_store__ptr_relaxed_mm_cid(v, expect, v2, expect2, newv, cpu);
	default:
		return -1;
	}
}

static inline __attribute__((always_inline))
int rseq_load_cbne_memcpy_store__ptr(enum rseq_mo rseq_mo, enum rseq_percpu_mode percpu_mode,
				 intptr_t *v, intptr_t expect,
				 void *dst, void *src, size_t len,
				 intptr_t newv, int cpu)
{
	switch (rseq_mo) {
	case RSEQ_MO_RELAXED:
		switch (percpu_mode) {
		case RSEQ_PERCPU_CPU_ID:
			return rseq_load_cbne_memcpy_store__ptr_relaxed_cpu_id(v, expect, dst, src, len, newv, cpu);
		case RSEQ_PERCPU_MM_CID:
			return rseq_load_cbne_memcpy_store__ptr_relaxed_mm_cid(v, expect, dst, src, len, newv, cpu);
		default:
			return -1;
		}
	case RSEQ_MO_RELEASE:
		switch (percpu_mode) {
		case RSEQ_PERCPU_CPU_ID:
			return rseq_load_cbne_memcpy_store__ptr_release_cpu_id(v, expect, dst, src, len, newv, cpu);
		case RSEQ_PERCPU_MM_CID:
			return rseq_load_cbne_memcpy_store__ptr_release_mm_cid(v, expect, dst, src, len, newv, cpu);
		default:
			return -1;
		}
	case RSEQ_MO_ACQUIRE:	/* Fallthrough */
	case RSEQ_MO_ACQ_REL:	/* Fallthrough */
	case RSEQ_MO_CONSUME:	/* Fallthrough */
	case RSEQ_MO_SEQ_CST:	/* Fallthrough */
	default:
		return -1;
	}
}

#ifdef __cplusplus
}
#endif

#endif  /* _RSEQ_RSEQ_H */
