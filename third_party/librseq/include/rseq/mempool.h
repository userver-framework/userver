/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */
/* SPDX-FileCopyrightText: 2024 Olivier Dion <odion@efficios.com> */

#ifndef _RSEQ_MEMPOOL_H
#define _RSEQ_MEMPOOL_H

#include <rseq/compiler.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>

/*
 * rseq/mempool.h: rseq memory pool allocator.
 *
 * The rseq memory pool allocator can be configured as either a global
 * allocator (default) or a per-CPU memory allocator.
 *
 * The rseq global memory allocator allows the application to request
 * memory pools of global memory each of containing objects of a
 * given size (rounded to next power of 2), reserving a given virtual
 * address size of the requested stride.
 *
 * The rseq per-CPU memory allocator allows the application the request
 * memory pools of CPU-Local memory each of containing objects of a
 * given size (rounded to next power of 2), reserving a given virtual
 * address size per CPU, for a given maximum number of CPUs.
 *
 * The per-CPU memory allocator is analogous to TLS (Thread-Local
 * Storage) memory: TLS is Thread-Local Storage, whereas the per-CPU
 * memory allocator provides CPU-Local Storage.
 *
 * Memory pool sets can be created by adding one or more pools into
 * them. They can be used to perform allocation of variable length
 * objects.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The percpu offset stride can be overridden by the user code.
 * The stride *must* match for all objects belonging to a given pool
 * between arguments to:
 *
 * - rseq_mempool_create(),
 * - rseq_percpu_ptr().
 * - rseq_mempool_percpu_free(),
 */
#define RSEQ_MEMPOOL_STRIDE	(1U << 16)	/* stride: 64kB */

/*
 * Tag pointers returned by:
 * - rseq_mempool_percpu_malloc(),
 * - rseq_mempool_percpu_zmalloc(),
 * - rseq_mempool_percpu_malloc_init(),
 * - rseq_mempool_set_percpu_malloc(),
 * - rseq_mempool_set_percpu_zmalloc(),
 * - rseq_mempool_set_percpu_malloc_init().
 *
 * and passed as parameter to:
 * - rseq_percpu_ptr(),
 * - rseq_mempool_percpu_free().
 *
 * with __rseq_percpu for use by static analyzers.
 */
#define __rseq_percpu

struct rseq_mempool_attr;
struct rseq_mempool;

/*
 * rseq_mempool_create: Create a memory pool.
 *
 * Create a memory pool for items of size @item_len (rounded to
 * next power of two).
 *
 * The @attr pointer used to specify the pool attributes. If NULL, use a
 * default attribute values. The @attr can be destroyed immediately
 * after rseq_mempool_create() returns. The caller keeps ownership
 * of @attr. Default attributes select a per-cpu mempool type.
 *
 * The argument @pool_name can be used to give a name to the pool for
 * debugging purposes. It can be NULL if no name is given.
 *
 * Returns a pointer to the created memory pool. Return NULL on error,
 * with errno set accordingly:
 *
 *   EINVAL: Invalid argument.
 *   ENOMEM: Not enough resources (memory or pool indexes) available to
 *           allocate pool.
 *
 * In addition, if the attr mmap callback fails, NULL is returned and
 * errno is propagated from the callback. The default callback can
 * return errno=ENOMEM.
 *
 * This API is MT-safe.
 */
struct rseq_mempool *rseq_mempool_create(const char *pool_name,
		size_t item_len, const struct rseq_mempool_attr *attr);

/*
 * rseq_mempool_destroy: Destroy a per-cpu memory pool.
 *
 * Destroy a per-cpu memory pool, unmapping its memory and removing the
 * pool entry from the global index. No pointers allocated from the
 * pool should be used when it is destroyed. This includes rseq_percpu_ptr().
 *
 * Argument @pool is a pointer to the per-cpu pool to destroy.
 *
 * Return values: 0 on success, -1 on error, with errno set accordingly:
 *
 *   ENOENT: Trying to free a pool which was not allocated.
 *
 * If the munmap_func callback fails, -1 is returned and errno is
 * propagated from the callback. The default callback can return
 * errno=EINVAL.
 *
 * This API is MT-safe.
 */
int rseq_mempool_destroy(struct rseq_mempool *pool);

/*
 * rseq_mempool_percpu_malloc: Allocate memory from a per-cpu pool.
 *
 * Allocate an item from a per-cpu @pool. The allocation will reserve
 * an item of the size specified by @item_len (rounded to next power of
 * two) at pool creation. This effectively reserves space for this item
 * on all CPUs.
 *
 * On success, return a "__rseq_percpu" encoded pointer to the pool
 * item. This encoded pointer is meant to be passed to rseq_percpu_ptr()
 * to be decoded to a valid address before being accessed.
 *
 * Return NULL (errno=ENOMEM) if there is not enough space left in the
 * pool to allocate an item.
 *
 * This API is MT-safe.
 */
void __rseq_percpu *rseq_mempool_percpu_malloc(struct rseq_mempool *pool);

/*
 * rseq_mempool_percpu_zmalloc: Allocate zero-initialized memory from a per-cpu pool.
 *
 * Allocate memory for an item within the pool, and zero-initialize its
 * memory on all CPUs. See rseq_mempool_percpu_malloc for details.
 *
 * This API is MT-safe.
 */
void __rseq_percpu *rseq_mempool_percpu_zmalloc(struct rseq_mempool *pool);

/*
 * rseq_mempool_percpu_malloc_init: Allocate initialized memory from a per-cpu pool.
 *
 * Allocate memory for an item within the pool, and initialize its
 * memory on all CPUs with content from @init_ptr of length @init_len.
 * See rseq_mempool_percpu_malloc for details.
 *
 * Return NULL (errno=ENOMEM) if there is not enough space left in the
 * pool to allocate an item. Return NULL (errno=EINVAL) if init_len is
 * larger than the pool item_len.
 *
 * This API is MT-safe.
 */
void __rseq_percpu *rseq_mempool_percpu_malloc_init(struct rseq_mempool *pool,
		void *init_ptr, size_t init_len);

/*
 * rseq_mempool_malloc: Allocate memory from a global pool.
 *
 * Wrapper to allocate memory from a global pool, which can be
 * used directly without per-cpu indexing. Would normally be used
 * with pools created with max_nr_cpus=1.
 */
static inline
void *rseq_mempool_malloc(struct rseq_mempool *pool)
{
	return (void *) rseq_mempool_percpu_malloc(pool);
}

/*
 * rseq_mempool_zmalloc: Allocate zero-initialized memory from a global pool.
 *
 * Wrapper to allocate memory from a global pool, which can be
 * used directly without per-cpu indexing. Would normally be used
 * with pools created with max_nr_cpus=1.
 */
static inline
void *rseq_mempool_zmalloc(struct rseq_mempool *pool)
{
	return (void *) rseq_mempool_percpu_zmalloc(pool);
}

/*
 * rseq_mempool_malloc_init: Allocate initialized memory from a global pool.
 *
 * Wrapper to allocate memory from a global pool, which can be
 * used directly without per-cpu indexing. Would normally be used
 * with pools created with max_nr_cpus=1.
 */
static inline
void *rseq_mempool_malloc_init(struct rseq_mempool *pool,
		void *init_ptr, size_t init_len)
{
	return (void *) rseq_mempool_percpu_malloc_init(pool, init_ptr, init_len);
}

/*
 * rseq_mempool_percpu_free: Free memory from a per-cpu pool.
 *
 * Free an item pointed to by @ptr from its per-cpu pool.
 *
 * The @ptr argument is a __rseq_percpu encoded pointer returned by
 * either:
 *
 * - rseq_mempool_percpu_malloc(),
 * - rseq_mempool_percpu_zmalloc(),
 * - rseq_mempool_percpu_malloc_init(),
 * - rseq_mempool_set_percpu_malloc(),
 * - rseq_mempool_set_percpu_zmalloc(),
 * - rseq_mempool_set_percpu_malloc_init().
 *
 * The @stride optional argument to rseq_percpu_free() is a configurable
 * stride, which must match the stride received by pool creation.
 * If the argument is not present, use the default RSEQ_MEMPOOL_STRIDE.
 *
 * This API is MT-safe.
 */
void librseq_mempool_percpu_free(void __rseq_percpu *ptr, size_t stride);

#define rseq_mempool_percpu_free(_ptr, _stride...)		\
	librseq_mempool_percpu_free(_ptr, RSEQ_PARAM_SELECT_ARG1(_, ##_stride, RSEQ_MEMPOOL_STRIDE))

/*
 * rseq_free: Free memory from a global pool.
 *
 * Free an item pointed to by @ptr from its global pool. Would normally
 * be used with pools created with max_nr_cpus=1.
 *
 * The @ptr argument is a pointer returned by either:
 *
 * - rseq_mempool_malloc(),
 * - rseq_mempool_zmalloc(),
 * - rseq_mempool_malloc_init(),
 * - rseq_mempool_set_malloc(),
 * - rseq_mempool_set_zmalloc(),
 * - rseq_mempool_set_malloc_init().
 *
 * The @stride optional argument to rseq_free() is a configurable
 * stride, which must match the stride received by pool creation. If
 * the argument is not present, use the default RSEQ_MEMPOOL_STRIDE.
 * The stride is needed even for a global pool to know the mapping
 * address range.
 *
 * This API is MT-safe.
 */
#define rseq_mempool_free(_ptr, _stride...)		\
	librseq_mempool_percpu_free((void __rseq_percpu *) _ptr, RSEQ_PARAM_SELECT_ARG1(_, ##_stride, RSEQ_MEMPOOL_STRIDE))

/*
 * rseq_percpu_ptr: Offset a per-cpu pointer for a given CPU.
 *
 * Offset a per-cpu pointer @ptr to get the associated pointer for the
 * given @cpu. The @ptr argument is a __rseq_percpu pointer returned by
 * either:
 *
 * - rseq_mempool_percpu_malloc(),
 * - rseq_mempool_percpu_zmalloc(),
 * - rseq_mempool_percpu_malloc_init(),
 * - rseq_mempool_set_percpu_malloc(),
 * - rseq_mempool_set_percpu_zmalloc(),
 * - rseq_mempool_set_percpu_malloc_init().
 *
 * The macro rseq_percpu_ptr() preserves the type of the @ptr parameter
 * for the returned pointer, but removes the __rseq_percpu annotation.
 *
 * The macro rseq_percpu_ptr() takes an optional @stride argument. If
 * the argument is not present, use the default RSEQ_MEMPOOL_STRIDE.
 * This must match the stride used for pool creation.
 *
 * This API is MT-safe.
 */
#define rseq_percpu_ptr(_ptr, _cpu, _stride...)		\
	((__typeof__(*(_ptr)) *) ((uintptr_t) (_ptr) +	\
		((unsigned int) (_cpu) *		\
			(uintptr_t) RSEQ_PARAM_SELECT_ARG1(_, ##_stride, RSEQ_MEMPOOL_STRIDE))))

/*
 * rseq_mempool_set_create: Create a pool set.
 *
 * Create a set of pools. Its purpose is to offer a memory allocator API
 * for variable-length items (e.g. variable length strings). When
 * created, the pool set has no pool. Pools can be created and added to
 * the set. One common approach would be to create pools for each
 * relevant power of two allocation size useful for the application.
 * Only one pool can be added to the pool set for each power of two
 * allocation size.
 *
 * Returns a pool set pointer on success, else returns NULL with
 * errno=ENOMEM (out of memory).
 *
 * This API is MT-safe.
 */
struct rseq_mempool_set *rseq_mempool_set_create(void);

/*
 * rseq_mempool_set_destroy: Destroy a pool set.
 *
 * Destroy a pool set and its associated resources. The pools that were
 * added to the pool set are destroyed as well.
 *
 * Returns 0 on success, -1 on failure (or partial failure), with errno
 * set by rseq_percpu_pool_destroy(). Using a pool set after destroy
 * failure is undefined.
 *
 * This API is MT-safe.
 */
int rseq_mempool_set_destroy(struct rseq_mempool_set *pool_set);

/*
 * rseq_mempool_set_add_pool: Add a pool to a pool set.
 *
 * Add a @pool to the @pool_set. On success, its ownership is handed
 * over to the pool set, so the caller should not destroy it explicitly.
 * Only one pool can be added to the pool set for each power of two
 * allocation size.
 *
 * Returns 0 on success, -1 on error with the following errno:
 * - EBUSY: A pool already exists in the pool set for this power of two
 *          allocation size.
 *
 * This API is MT-safe.
 */
int rseq_mempool_set_add_pool(struct rseq_mempool_set *pool_set,
		struct rseq_mempool *pool);

/*
 * rseq_mempool_set_percpu_malloc: Allocate memory from a per-cpu pool set.
 *
 * Allocate an item from a per-cpu @pool. The allocation will reserve
 * an item of the size specified by @len (rounded to next power of
 * two). This effectively reserves space for this item on all CPUs.
 *
 * The space reservation will search for the smallest pool within
 * @pool_set which respects the following conditions:
 *
 * - it has an item size large enough to fit @len,
 * - it has space available.
 *
 * On success, return a "__rseq_percpu" encoded pointer to the pool
 * item. This encoded pointer is meant to be passed to rseq_percpu_ptr()
 * to be decoded to a valid address before being accessed.
 *
 * Return NULL (errno=ENOMEM) if there is not enough space left in the
 * pool to allocate an item.
 *
 * This API is MT-safe.
 */
void __rseq_percpu *rseq_mempool_set_percpu_malloc(struct rseq_mempool_set *pool_set, size_t len);

/*
 * rseq_mempool_set_percpu_zmalloc: Allocate zero-initialized memory from a per-cpu pool set.
 *
 * Allocate memory for an item within the pool, and zero-initialize its
 * memory on all CPUs. See rseq_mempool_set_percpu_malloc for details.
 *
 * This API is MT-safe.
 */
void __rseq_percpu *rseq_mempool_set_percpu_zmalloc(struct rseq_mempool_set *pool_set, size_t len);

/*
 * rseq_mempool_set_percpu_malloc_init: Allocate initialized memory from a per-cpu pool set.
 *
 * Allocate memory for an item within the pool, and initialize its
 * memory on all CPUs with content from @init_ptr of length @len.
 * See rseq_mempool_set_percpu_malloc for details.
 *
 * This API is MT-safe.
 */
void __rseq_percpu *rseq_mempool_set_percpu_malloc_init(struct rseq_mempool_set *pool_set,
		void *init_ptr, size_t len);

/*
 * rseq_mempool_set_malloc: Allocate memory from a global pool set.
 *
 * Wrapper to allocate memory from a global pool, which can be
 * used directly without per-cpu indexing. Would normally be used
 * with pools created with max_nr_cpus=1.
 */
static inline
void *rseq_mempool_set_malloc(struct rseq_mempool_set *pool_set, size_t len)
{
	return (void *) rseq_mempool_set_percpu_malloc(pool_set, len);
}

/*
 * rseq_mempool_set_zmalloc: Allocate zero-initialized memory from a global pool set.
 *
 * Wrapper to allocate memory from a global pool, which can be
 * used directly without per-cpu indexing. Would normally be used
 * with pools created with max_nr_cpus=1.
 */
static inline
void *rseq_mempool_set_zmalloc(struct rseq_mempool_set *pool_set, size_t len)
{
	return (void *) rseq_mempool_set_percpu_zmalloc(pool_set, len);
}

/*
 * rseq_mempool_set_malloc_init: Allocate initialized memory from a global pool set.
 *
 * Wrapper to allocate memory from a global pool, which can be
 * used directly without per-cpu indexing. Would normally be used
 * with pools created with max_nr_cpus=1.
 */
static inline
void *rseq_mempool_set_malloc_init(struct rseq_mempool_set *pool_set, void *init_ptr, size_t len)
{
	return (void *) rseq_mempool_set_percpu_malloc_init(pool_set, init_ptr, len);
}

/*
 * rseq_mempool_attr_create: Create a pool attribute structure.
 */
struct rseq_mempool_attr *rseq_mempool_attr_create(void);

/*
 * rseq_mempool_attr_destroy: Destroy a pool attribute structure.
 */
void rseq_mempool_attr_destroy(struct rseq_mempool_attr *attr);

/*
 * rseq_mempool_attr_set_init: Set pool attribute structure memory init functions.
 *
 * The @init_func callback used to initialized memory after allocation
 * for the pool. The @cpu argument of @init_func, if >= 0, is the cpu to
 * which belongs the range starting at @addr of length @len. If cpu is
 * -1, it means the range belongs to a global pool. The @init_func
 * callback must return 0 on success, -1 on error with errno set. If
 * @init_func returns failure, the allocation of the pool memory fails,
 * which either causes the pool creation to fail or memory allocation to
 * fail (for extensible memory pools).
 *
 * The @init_priv argument is a private data pointer passed to the
 * @init_func callback.
 *
 * Returns 0 on success, -1 with errno=EINVAL if arguments are invalid.
 */
int rseq_mempool_attr_set_init(struct rseq_mempool_attr *attr,
		int (*init_func)(void *priv, void *addr, size_t len, int cpu),
		void *init_priv);

/*
 * rseq_mempool_attr_set_robust: Set pool robust attribute.
 *
 * The robust pool attribute enables runtime validation of the pool:
 *
 *   - Check for double-free of pointers.
 *
 *   - Detect memory leaks on pool destruction.
 *
 *   - Detect free-list corruption on pool destruction.
 *
 *   - Detect poison value corruption on allocation and pool destruction.
 *
 * There is a marginal runtime overhead on malloc/free operations.
 *
 * The memory overhead is (pool->percpu_len / pool->item_len) / CHAR_BIT
 * bytes, plus one additional stride range for a separate free list,
 * over the lifetime of the pool.
 *
 * Returns 0 on success, -1 with errno=EINVAL if arguments are invalid.
 */
int rseq_mempool_attr_set_robust(struct rseq_mempool_attr *attr);

/*
 * rseq_mempool_attr_set_percpu: Set pool type as percpu.
 *
 * A pool created with this type is a per-cpu memory pool. The reserved
 * allocation size is @stride, and the maximum CPU value expected
 * is (@max_nr_cpus - 1). A @stride of 0 uses the default
 * RSEQ_MEMPOOL_STRIDE.
 *
 * Returns 0 on success, -1 with errno=EINVAL if arguments are invalid.
 */
int rseq_mempool_attr_set_percpu(struct rseq_mempool_attr *attr,
		size_t stride, int max_nr_cpus);

/*
 * rseq_mempool_attr_set_global: Set pool type as global.
 *
 * A pool created with this type is a global memory pool. The reserved
 * allocation size is @stride. A @stride of 0 uses the default
 * RSEQ_MEMPOOL_STRIDE.
 *
 * Returns 0 on success, -1 with errno=EINVAL if arguments are invalid.
 */
int rseq_mempool_attr_set_global(struct rseq_mempool_attr *attr, size_t stride);

/*
 * rseq_mempool_attr_set_max_nr_ranges: Set upper-limit to range allocation.
 *
 * Set an upper-limit to range allocation. A @max_nr_ranges value of
 * 0 means no limit (default).
 *
 * Returns 0 on success, -1 with errno=EINVAL if arguments are invalid.
 */
int rseq_mempool_attr_set_max_nr_ranges(struct rseq_mempool_attr *attr,
		unsigned long max_nr_ranges);

/*
 * rseq_mempool_attr_set_poison: Set pool poison value.
 *
 * Set a poison value to be set over freed pool entries. This can be
 * used to anonymize freed memory, and for memory corruption checks
 * with the robust attribute.
 *
 * Returns 0 on success, -1 with errno=EINVAL if arguments are invalid.
 */
int rseq_mempool_attr_set_poison(struct rseq_mempool_attr *attr,
		uintptr_t poison);

enum rseq_mempool_populate_policy {
	/*
	 * RSEQ_MEMPOOL_POPULATE_COW_INIT (default):
	 *   Rely on copy-on-write (COW) of per-cpu pages to populate
	 *   per-cpu pages from the initial values pages on first write.
	 *   Note that this type of pool cannot be accessed from
	 *   children processes across fork. It is however valid to
	 *   destroy a pool from a child process after a fork to free
	 *   its remaining resources.
	 */
	RSEQ_MEMPOOL_POPULATE_COW_INIT = 0,

	/*
	 * RSEQ_MEMPOOL_POPULATE_COW_ZERO:
	 *   Rely on copy-on-write (COW) of per-cpu pages to populate
	 *   per-cpu pages from the zero page on first write. As long
	 *   as the user only uses malloc, zmalloc, or malloc_init with
	 *   zeroed content to allocate items, it does not trigger COW
	 *   of per-cpu pages, leaving in place the zero page until an
	 *   active CPU writes to its per-cpu item. The recommended (and
	 *   default) poison value for this pool policy is 0 to prevent
	 *   useless COW page allocation.
	 */
	RSEQ_MEMPOOL_POPULATE_COW_ZERO = 1,
};

/*
 * rseq_mempool_attr_set_populate_policy: Set pool page populate policy.
 *
 * Set page populate policy for the mempool.
 *
 * Returns 0 on success, -1 with errno=EINVAL if arguments are invalid.
 */
int rseq_mempool_attr_set_populate_policy(struct rseq_mempool_attr *attr,
		enum rseq_mempool_populate_policy policy);

/*
 * rseq_mempool_range_init_numa: NUMA initialization helper for memory range.
 *
 * Helper which can be used from mempool_attr @init_func to move a CPU
 * memory range to the NUMA node associated to its topology.
 *
 * Returns 0 on success, -1 with errno set by move_pages(2) on error.
 * Returns -1, errno=ENOSYS if NUMA support is not present.
 */
int rseq_mempool_range_init_numa(void *addr, size_t len, int cpu, int numa_flags);

/*
 * rseq_mempool_get_max_nr_cpus: Get the max_nr_cpus value configured for a pool.
 *
 * Returns a value >= 0 for a per-cpu pool.
 * Returns -1, errno=EINVAL if the mempool is NULL or if the pool has a
 * global pool type.
 */
int rseq_mempool_get_max_nr_cpus(struct rseq_mempool *mempool);

#ifdef __cplusplus
}
#endif

#endif /* _RSEQ_MEMPOOL_H */
