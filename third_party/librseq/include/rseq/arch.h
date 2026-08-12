/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2016-2022 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq/arch.h
 */

#include <rseq/thread-pointer.h>

/*
 * Architecture detection using compiler defines.
 *
 * The following defines are used internally for architecture specific code.
 *
 * URCU_ARCH_X86 : All x86 variants 32 and 64 bits
 *   URCU_ARCH_I386 : Specific to the i386
 *   URCU_ARCH_AMD64 : All 64 bits x86 variants
 *
 * URCU_ARCH_PPC : All PowerPC variants 32 and 64 bits
 *   URCU_ARCH_PPC64 : Specific to 64 bits variants
 *
 * URCU_ARCH_S390 : All IBM s390 / s390x variants
 *   URCU_ARCH_S390X : Specific to z/Architecture 64 bits
 *
 * URCU_ARCH_ARM : All ARM 32 bits variants
 * URCU_ARCH_AARCH64 : All ARM 64 bits variants
 * URCU_ARCH_MIPS : All MIPS variants
 * URCU_ARCH_RISCV : All RISC-V variants
 */

#ifndef _RSEQ_ARCH_H
#define _RSEQ_ARCH_H

#if (defined(__amd64__) \
	|| defined(__amd64) \
	|| defined(__x86_64__) \
	|| defined(__x86_64))

#define RSEQ_ARCH_X86 1
#define RSEQ_ARCH_AMD64 1
#include <rseq/arch/x86.h>

#elif (defined(__i386__) || defined(__i386))

#define RSEQ_ARCH_X86 1
#include <rseq/arch/x86.h>

#elif (defined(__arm__) || defined(__arm))

#define RSEQ_ARCH_ARM 1
#include <rseq/arch/arm.h>

#elif defined(__aarch64__)

#define RSEQ_ARCH_AARCH64 1
#include <rseq/arch/aarch64.h>

#elif (defined(__powerpc64__) || defined(__ppc64__))

#define RSEQ_ARCH_PPC 1
#define RSEQ_ARCH_PPC64 1
#include <rseq/arch/ppc.h>

#elif (defined(__powerpc__) \
	|| defined(__powerpc) \
	|| defined(__ppc__))

#define RSEQ_ARCH_PPC 1
#include <rseq/arch/ppc.h>

#elif (defined(__mips__) || defined(__mips))

#define RSEQ_ARCH_MIPS 1
#include <rseq/arch/mips.h>

#elif defined(__s390__)

# if (defined(__s390x__) || defined(__zarch__))
#  define RSEQ_ARCH_S390X 1
# endif

#define RSEQ_ARCH_S390 1
#include <rseq/arch/s390.h>

#elif defined(__riscv)

#define RSEQ_ARCH_RISCV 1
#include <rseq/arch/riscv.h>

#else
#error "Cannot build: unrecognized architecture, see <rseq/arch.h>."
#endif

#endif /* _RSEQ_ARCH_H */
