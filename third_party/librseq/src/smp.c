// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2011-2012 Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
 * Copyright (C) 2019 Michael Jeanson <mjeanson@efficios.com>
 */

#define _LGPL_SOURCE
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <rseq/compiler.h>

#include "smp.h"

#define __max(a,b) ((a)>(b)?(a):(b))

#define RSEQ_CPUMASK_SIZE	4096

static int possible_cpus_array_len_cache;

static
int _get_max_cpuid_from_sysfs(const char *path)
{
	long max_cpuid = -1;

	DIR *cpudir;
	struct dirent *entry;

	assert(path);

	cpudir = opendir(path);
	if (cpudir == NULL)
		goto end;

	/*
	 * Iterate on all directories named "cpu" followed by an integer.
	 */
	while ((entry = readdir(cpudir))) {
		if (entry->d_type == DT_DIR &&
			strncmp(entry->d_name, "cpu", 3) == 0) {

			char *endptr;
			long cpu_id;

			cpu_id = strtol(entry->d_name + 3, &endptr, 10);
			if ((cpu_id < LONG_MAX) && (endptr != entry->d_name + 3)
					&& (*endptr == '\0')) {
				if (cpu_id > max_cpuid)
					max_cpuid = cpu_id;
			}
		}
	}

	if (closedir(cpudir))
		perror("closedir");

	/*
	 * If the max CPU id is out of bound, set it to -1 so it results in a
	 * CPU num of 0.
	 */
	if (max_cpuid < 0 || max_cpuid > INT_MAX)
		max_cpuid = -1;

end:
	return max_cpuid;
}

/*
 * Get the highest CPU id from sysfs.
 *
 * Iterate on all the folders in "/sys/devices/system/cpu" that start with
 * "cpu" followed by an integer, keep the highest CPU id encountered during
 * this iteration and add 1 to get a number of CPUs.
 *
 * Returns the highest CPU id, or -1 on error.
 */
static
int get_max_cpuid_from_sysfs(void)
{
	return _get_max_cpuid_from_sysfs("/sys/devices/system/cpu");
}

/*
 * As a fallback to parsing the CPU mask in "/sys/devices/system/cpu/possible",
 * iterate on all the folders in "/sys/devices/system/cpu" that start with
 * "cpu" followed by an integer, keep the highest CPU id encountered during
 * this iteration and add 1 to get a number of CPUs.
 *
 * Then get the value from sysconf(_SC_NPROCESSORS_CONF) as a fallback and
 * return the highest one.
 *
 * On Linux, using the value from sysconf can be unreliable since the way it
 * counts CPUs varies between C libraries and even between versions of the same
 * library. If we used it directly, getcpu() could return a value greater than
 * this sysconf, in which case the arrays indexed by processor would overflow.
 *
 * As another example, the MUSL libc implementation of the _SC_NPROCESSORS_CONF
 * sysconf does not return the number of configured CPUs in the system but
 * relies on the cpu affinity mask of the current task.
 *
 * Returns 0 or less on error.
 */
static
int get_num_possible_cpus_fallback(void)
{
	/*
	 * Get the sysconf value as a last resort. Keep the highest number.
	 */
	return __max(sysconf(_SC_NPROCESSORS_CONF), get_max_cpuid_from_sysfs() + 1);
}

/*
 * Get a CPU mask string from sysfs.
 *
 * buf: the buffer where the mask will be read.
 * max_bytes: the maximum number of bytes to write in the buffer.
 * path: file path to read the mask from.
 *
 * Returns the number of bytes read or -1 on error.
 */
static
int get_cpu_mask_from_sysfs(char *buf, size_t max_bytes, const char *path)
{
	ssize_t bytes_read = 0;
	size_t total_bytes_read = 0;
	int fd = -1, ret = -1;

	assert(path);

	if (buf == NULL)
		goto end;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		goto end;

	do {
		bytes_read = read(fd, buf + total_bytes_read,
				max_bytes - total_bytes_read);

		if (bytes_read < 0) {
			if (errno == EINTR) {
				continue;	/* retry operation */
			} else {
				goto end;
			}
		}

		total_bytes_read += bytes_read;
		assert(total_bytes_read <= max_bytes);
	} while (max_bytes > total_bytes_read && bytes_read != 0);

	/*
	 * Make sure the mask read is a null terminated string.
	 */
	if (total_bytes_read < max_bytes)
		buf[total_bytes_read] = '\0';
	else
		buf[max_bytes - 1] = '\0';

	if (total_bytes_read > INT_MAX)
		goto end;
	ret = (int) total_bytes_read;
end:
	if (fd >= 0 && close(fd) < 0)
		perror("close");
	return ret;
}

/*
 * Get the CPU possible mask string from sysfs.
 *
 * buf: the buffer where the mask will be read.
 * max_bytes: the maximum number of bytes to write in the buffer.
 *
 * Returns the number of bytes read or -1 on error.
 */
static
int get_possible_cpu_mask_from_sysfs(char *buf, size_t max_bytes)
{
	return get_cpu_mask_from_sysfs(buf, max_bytes,
			"/sys/devices/system/cpu/possible");
}

/*
 * Get the highest CPU id from a CPU mask.
 *
 * pmask: the mask to parse.
 * len: the len of the mask excluding '\0'.
 *
 * Returns the highest CPU id from the mask or -1 on error.
 */
static
int get_max_cpuid_from_mask(const char *pmask, size_t len)
{
	ssize_t i;
	unsigned long cpu_index;
	char *endptr;

	/* We need at least one char to read */
	if (len < 1)
		goto error;

	/* Start from the end to read the last CPU index. */
	for (i = len - 1; i > 0; i--) {
		/* Break when we hit the first separator. */
		if ((pmask[i] == ',') || (pmask[i] == '-')) {
			i++;
			break;
		}
	}

	cpu_index = strtoul(&pmask[i], &endptr, 10);

	if ((&pmask[i] != endptr) && (cpu_index < INT_MAX))
		return (int) cpu_index;

error:
	return -1;
}

static void update_possible_cpus_array_len_cache(void)
{
	char buf[RSEQ_CPUMASK_SIZE];
	int ret;

	/* Get the possible cpu mask from sysfs, fallback to sysconf. */
	ret = get_possible_cpu_mask_from_sysfs((char *) &buf, RSEQ_CPUMASK_SIZE);
	if (ret <= 0)
		goto fallback;

	/* Parse the possible cpu mask, on failure fallback to sysconf. */
	ret = get_max_cpuid_from_mask((char *) &buf, ret);
	if (ret >= 0) {
		/* Add 1 to convert from max cpuid to an array len. */
		ret++;
		goto end;
	}

fallback:
	/* Fallback to sysconf. */
	ret = get_num_possible_cpus_fallback();

end:
	/* If all methods failed, don't store the value. */
	if (ret < 1)
		return;

	possible_cpus_array_len_cache = ret;
}

/*
 * Returns the length of an array that could contain a per-CPU element for each
 * possible CPU id for the lifetime of the process.
 *
 * We currently assume CPU ids are contiguous up the maximum CPU id.
 *
 * If the cache is not yet initialized, get the value from
 * "/sys/devices/system/cpu/possible" or fallback to sysconf and cache it.
 *
 * If all methods fail, don't populate the cache and return 0.
 */
int get_possible_cpus_array_len(void)
{
	if (rseq_unlikely(!possible_cpus_array_len_cache))
		update_possible_cpus_array_len_cache();

	return possible_cpus_array_len_cache;
}
