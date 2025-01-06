/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq/arch/generic/common.h: Common architecture support macros.
 */

#ifndef _RSEQ_GENERIC_COMMON_H
#define _RSEQ_GENERIC_COMMON_H

/*
 * Define the rseq critical section descriptor fields.
 */
 #define __RSEQ_ASM_DEFINE_CS_FIELDS(version, flags,			\
				start_ip, post_commit_offset, abort_ip)	\
		RSEQ_ASM_U32(__rseq_str(version)) "\n\t" 		\
		RSEQ_ASM_U32(__rseq_str(flags)) "\n\t" 			\
		RSEQ_ASM_U64_PTR(__rseq_str(start_ip)) "\n\t"		\
		RSEQ_ASM_U64_PTR(__rseq_str(post_commit_offset)) "\n\t" \
		RSEQ_ASM_U64_PTR(__rseq_str(abort_ip))

/* Only used in RSEQ_ASM_DEFINE_TABLE. */
#define __RSEQ_ASM_DEFINE_TABLE(label, version, flags,			\
				start_ip, post_commit_offset, abort_ip)	\
		".pushsection __rseq_cs, \"aw\"\n\t"			\
		".balign 32\n\t"					\
		__rseq_str(label) ":\n\t"				\
		__RSEQ_ASM_DEFINE_CS_FIELDS(version, flags,		\
			start_ip, post_commit_offset, abort_ip) "\n\t"	\
		RSEQ_ASM_U32(__rseq_str(version)) "\n\t" 		\
		RSEQ_ASM_U32(__rseq_str(flags)) "\n\t" 			\
		RSEQ_ASM_U64_PTR(__rseq_str(start_ip)) "\n\t"		\
		RSEQ_ASM_U64_PTR(__rseq_str(post_commit_offset)) "\n\t" \
		RSEQ_ASM_U64_PTR(__rseq_str(abort_ip)) "\n\t"		\
		".popsection\n\t"					\
		".pushsection __rseq_cs_ptr_array, \"aw\"\n\t"		\
		RSEQ_ASM_U64_PTR(__rseq_str(label) "b") "\n\t"		\
		".popsection\n\t"

/*
 * Define an rseq critical section structure of version 0 with no flags.
 *
 *  @label:
 *    Local label for the beginning of the critical section descriptor
 *    structure.
 *  @start_ip:
 *    Pointer to the first instruction of the sequence of consecutive assembly
 *    instructions.
 *  @post_commit_ip:
 *    Pointer to the instruction after the last instruction of the sequence of
 *    consecutive assembly instructions.
 *  @abort_ip:
 *    Pointer to the instruction where to move the execution flow in case of
 *    abort of the sequence of consecutive assembly instructions.
 */
#define RSEQ_ASM_DEFINE_TABLE(label, start_ip, post_commit_ip, abort_ip) \
	__RSEQ_ASM_DEFINE_TABLE(label, 0x0, 0x0, start_ip,		\
				(post_commit_ip) - (start_ip), abort_ip)

/*
 * Define the @exit_ip pointer as an exit point for the sequence of consecutive
 * assembly instructions at @start_ip.
 *
 *  @start_ip:
 *    Pointer to the first instruction of the sequence of consecutive assembly
 *    instructions.
 *  @exit_ip:
 *    Pointer to an exit point instruction.
 *
 * Exit points of a rseq critical section consist of all instructions outside
 * of the critical section where a critical section can either branch to or
 * reach through the normal course of its execution. The abort IP and the
 * post-commit IP are already part of the __rseq_cs section and should not be
 * explicitly defined as additional exit points. Knowing all exit points is
 * useful to assist debuggers stepping over the critical section.
 */
#define RSEQ_ASM_DEFINE_EXIT_POINT(start_ip, exit_ip)			\
		".pushsection __rseq_exit_point_array, \"aw\"\n\t"	\
		RSEQ_ASM_U64_PTR(__rseq_str(start_ip)) "\n\t"		\
		RSEQ_ASM_U64_PTR(__rseq_str(exit_ip)) "\n\t"		\
		".popsection\n\t"

#endif
