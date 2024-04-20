/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2021 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq/compiler.h
 *
 * Work-around asm goto compiler bugs.
 */

#ifndef RSEQ_COMPILER_H
#define RSEQ_COMPILER_H

#if defined __cplusplus
# include <type_traits>	/* for std::remove_cv */
#endif

/*
 * gcc prior to 4.8.2 miscompiles asm goto.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=58670
 *
 * gcc prior to 8.1.0 miscompiles asm goto at O1.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103908
 *
 * clang prior to version 13.0.1 miscompiles asm goto at O2.
 * https://github.com/llvm/llvm-project/issues/52735
 *
 * Work around these issues by adding a volatile inline asm with
 * memory clobber in the fallthrough after the asm goto and at each
 * label target.  Emit this for all compilers in case other similar
 * issues are found in the future.
 */
#define rseq_after_asm_goto()	__asm__ __volatile__ ("" : : : "memory")

/* Combine two tokens. */
#define RSEQ__COMBINE_TOKENS(_tokena, _tokenb)	\
	_tokena##_tokenb
#define RSEQ_COMBINE_TOKENS(_tokena, _tokenb)	\
	RSEQ__COMBINE_TOKENS(_tokena, _tokenb)

#if defined(__SIZEOF_LONG__)
#define RSEQ_BITS_PER_LONG	(__SIZEOF_LONG__ * 8)
#elif defined(_LP64)
#define RSEQ_BITS_PER_LONG	64
#else
#define RSEQ_BITS_PER_LONG	32
#endif

#ifdef __cplusplus
#define rseq_unqual_scalar_typeof(x)					\
	std::remove_cv<std::remove_reference<decltype(x)>::type>::type
#else
#define rseq_scalar_type_to_expr(type)					\
	unsigned type: (unsigned type)0,				\
	signed type: (signed type)0

/*
 * Use C11 _Generic to express unqualified type from expression. This removes
 * volatile qualifier from expression type.
 */
#define rseq_unqual_scalar_typeof(x)					\
	__typeof__(							\
		_Generic((x),						\
			char: (char)0,					\
			rseq_scalar_type_to_expr(char),			\
			rseq_scalar_type_to_expr(short),		\
			rseq_scalar_type_to_expr(int),			\
			rseq_scalar_type_to_expr(long),			\
			rseq_scalar_type_to_expr(long long),		\
			default: (x)					\
		)							\
	)
#endif

#endif  /* RSEQ_COMPILER_H_ */
