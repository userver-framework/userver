// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2016 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syscall.h>
#include <assert.h>
#include <signal.h>
#include <limits.h>
#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/auxv.h>
#include <linux/auxvec.h>

#include <rseq/rseq.h>
#include "smp.h"

#ifndef AT_RSEQ_FEATURE_SIZE
# define AT_RSEQ_FEATURE_SIZE		27
#endif

#ifndef AT_RSEQ_ALIGN
# define AT_RSEQ_ALIGN			28
#endif

static __attribute__((constructor))
void rseq_init(void);

static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static int init_done;

static const ptrdiff_t *libc_rseq_offset_p;
static const unsigned int *libc_rseq_size_p;
static const unsigned int *libc_rseq_flags_p;

/* Offset from the thread pointer to the rseq area. */
ptrdiff_t rseq_offset;

/*
 * Size of the active rseq feature set. 0 if the registration was
 * unsuccessful.
 */
unsigned int rseq_size = -1U;

/* Flags used during rseq registration. */
unsigned int rseq_flags;

static int rseq_ownership;

/* Allocate a large area for the TLS. */
#define RSEQ_THREAD_AREA_ALLOC_SIZE	1024

/* Original struct rseq feature size is 20 bytes. */
#define ORIG_RSEQ_FEATURE_SIZE		20

/* Original struct rseq allocation size is 32 bytes. */
#define ORIG_RSEQ_ALLOC_SIZE		32

/*
 * The alignment on RSEQ_THREAD_AREA_ALLOC_SIZE guarantees that the
 * rseq_abi structure allocated size is at least
 * RSEQ_THREAD_AREA_ALLOC_SIZE bytes to hold extra space for yet unknown
 * kernel rseq extensions.
 */
static
__thread struct rseq_abi __rseq_abi __attribute__((tls_model("initial-exec"), aligned(RSEQ_THREAD_AREA_ALLOC_SIZE))) = {
	.cpu_id = RSEQ_ABI_CPU_ID_UNINITIALIZED,
};

static int sys_rseq(struct rseq_abi *rseq_abi, uint32_t rseq_len,
		    int flags, uint32_t sig)
{
	return syscall(__NR_rseq, rseq_abi, rseq_len, flags, sig);
}

static int sys_getcpu(unsigned int *cpu, unsigned int *node)
{
	return syscall(__NR_getcpu, cpu, node, NULL);
}

bool rseq_available(unsigned int query)
{
	int rc;

	switch (query) {
	case RSEQ_AVAILABLE_QUERY_KERNEL:
		rc = sys_rseq(NULL, 0, 0, 0);
		if (rc != -1)
			abort();
		switch (errno) {
		case ENOSYS:
			break;
		case EINVAL:
			return true;
		default:
			abort();
		}
		break;
	case RSEQ_AVAILABLE_QUERY_LIBC:
		if (rseq_size && !rseq_ownership)
			return true;
		break;
	default:
		break;
	}
	return false;
}

/* The rseq areas need to be at least 32 bytes. */
static
unsigned int get_rseq_min_alloc_size(void)
{
	unsigned int alloc_size = rseq_size;

	if (alloc_size < ORIG_RSEQ_ALLOC_SIZE)
		alloc_size = ORIG_RSEQ_ALLOC_SIZE;
	return alloc_size;
}

/*
 * Return the feature size supported by the kernel.
 *
 * Depending on the value returned by getauxval(AT_RSEQ_FEATURE_SIZE):
 *
 * 0:   Return ORIG_RSEQ_FEATURE_SIZE (20)
 * > 0: Return the value from getauxval(AT_RSEQ_FEATURE_SIZE).
 *
 * It should never return a value below ORIG_RSEQ_FEATURE_SIZE.
 */
static
unsigned int get_rseq_kernel_feature_size(void)
{
	unsigned long auxv_rseq_feature_size, auxv_rseq_align;

	auxv_rseq_align = getauxval(AT_RSEQ_ALIGN);
	assert(!auxv_rseq_align || auxv_rseq_align <= RSEQ_THREAD_AREA_ALLOC_SIZE);

	auxv_rseq_feature_size = getauxval(AT_RSEQ_FEATURE_SIZE);
	assert(!auxv_rseq_feature_size || auxv_rseq_feature_size <= RSEQ_THREAD_AREA_ALLOC_SIZE);
	if (auxv_rseq_feature_size)
		return auxv_rseq_feature_size;
	else
		return ORIG_RSEQ_FEATURE_SIZE;
}

int rseq_register_current_thread(void)
{
	int rc;

	rseq_init();

	if (!rseq_ownership) {
		/* Treat libc's ownership as a successful registration. */
		return 0;
	}
	rc = sys_rseq(&__rseq_abi, get_rseq_min_alloc_size(), 0, RSEQ_SIG);
	if (rc) {
		/*
		 * After at least one thread has registered successfully
		 * (rseq_size > 0), the registration of other threads should
		 * never fail.
		 */
		if (RSEQ_READ_ONCE(rseq_size) > 0) {
			/* Incoherent success/failure within process. */
			abort();
		}
		return -1;
	}
	assert(rseq_current_cpu_raw() >= 0);

	/*
	 * The first thread to register sets the rseq_size to mimic the libc
	 * behavior.
	 */
	if (RSEQ_READ_ONCE(rseq_size) == 0) {
		RSEQ_WRITE_ONCE(rseq_size, get_rseq_kernel_feature_size());
	}

	return 0;
}

int rseq_unregister_current_thread(void)
{
	int rc;

	if (!rseq_ownership) {
		/* Treat libc's ownership as a successful unregistration. */
		return 0;
	}
	rc = sys_rseq(&__rseq_abi, get_rseq_min_alloc_size(), RSEQ_ABI_FLAG_UNREGISTER, RSEQ_SIG);
	if (rc)
		return -1;
	return 0;
}

/*
 * Initialize the public symbols for the rseq offset, size, feature size and
 * flags prior to registering threads. If glibc owns the registration, get the
 * values from its public symbols.
 */
static
void rseq_init(void)
{
	/*
	 * Ensure initialization is only done once. Use load-acquire to
	 * observe the initialization performed by a concurrently
	 * running thread.
	 */
	if (rseq_smp_load_acquire(&init_done))
		return;

	/*
	 * Take the mutex, check the initialization flag again and atomically
	 * set it to ensure we are the only thread doing the initialization.
	 */
	pthread_mutex_lock(&init_lock);
	if (init_done)
		goto unlock;

	/*
	 * Check for glibc rseq support, if the 3 public symbols are found and
	 * the rseq_size is not zero, glibc owns the registration.
	 */
	libc_rseq_offset_p = dlsym(RTLD_NEXT, "__rseq_offset");
	libc_rseq_size_p = dlsym(RTLD_NEXT, "__rseq_size");
	libc_rseq_flags_p = dlsym(RTLD_NEXT, "__rseq_flags");
	if (libc_rseq_size_p && libc_rseq_offset_p && libc_rseq_flags_p &&
			*libc_rseq_size_p != 0) {
		unsigned int libc_rseq_size;

		/* rseq registration owned by glibc */
		rseq_offset = *libc_rseq_offset_p;
		libc_rseq_size = *libc_rseq_size_p;
		rseq_flags = *libc_rseq_flags_p;

		/*
		 * Previous versions of glibc expose the value
		 * 32 even though the kernel only supported 20
		 * bytes initially. Therefore treat 32 as a
		 * special-case. glibc 2.40 exposes a 20 bytes
		 * __rseq_size without using getauxval(3) to
		 * query the supported size, while still allocating a 32
		 * bytes area. Also treat 20 as a special-case.
		 *
		 * Special-cases are handled by using the following
		 * value as active feature set size:
		 *
		 *   rseq_size = min(32, get_rseq_kernel_feature_size())
		 */
		switch (libc_rseq_size) {
		case ORIG_RSEQ_FEATURE_SIZE:	/* Fallthrough. */
		case ORIG_RSEQ_ALLOC_SIZE:
		{
			unsigned int rseq_kernel_feature_size = get_rseq_kernel_feature_size();

			if (rseq_kernel_feature_size < ORIG_RSEQ_ALLOC_SIZE)
				rseq_size = rseq_kernel_feature_size;
			else
				rseq_size = ORIG_RSEQ_ALLOC_SIZE;
			break;
		}
		default:
			/* Otherwise just use the __rseq_size from libc as rseq_size. */
			rseq_size = libc_rseq_size;
			break;
		}
		goto unlock;
	}

	/* librseq owns the registration */
	rseq_ownership = 1;

	/* Calculate the offset of the rseq area from the thread pointer. */
	rseq_offset = (uintptr_t)&__rseq_abi - (uintptr_t)rseq_thread_pointer();

	/* rseq flags are deprecated, always set to 0. */
	rseq_flags = 0;

	/*
	 * Set the size to 0 until at least one thread registers to mimic the
	 * libc behavior.
	 */
	rseq_size = 0;
	/*
	 * Set init_done with store-release, to make sure concurrently
	 * running threads observe the initialized state.
	 */
	rseq_smp_store_release(&init_done, 1);
unlock:
	pthread_mutex_unlock(&init_lock);
}

static __attribute__((destructor))
void rseq_exit(void)
{
	if (!rseq_ownership)
		return;
	rseq_offset = 0;
	rseq_size = -1U;
	rseq_ownership = 0;
}

int32_t rseq_fallback_current_cpu(void)
{
	int32_t cpu;

	cpu = sched_getcpu();
	if (cpu < 0) {
		perror("sched_getcpu()");
		abort();
	}
	return cpu;
}

int32_t rseq_fallback_current_node(void)
{
	uint32_t cpu_id, node_id;
	int ret;

	ret = sys_getcpu(&cpu_id, &node_id);
	if (ret) {
		perror("sys_getcpu()");
		return ret;
	}
	return (int32_t) node_id;
}

int rseq_get_max_nr_cpus(void)
{
	return get_possible_cpus_array_len();
}
