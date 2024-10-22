/* SPDX-License-Identifier: MIT */
/* SPDX-FileCopyrightText: 2024 Mathieu Desnoyers <mathieu.desnoyers@efficios.com> */

/*
 * rseq/pseudocode.h
 *
 * This file contains the pseudo-code of rseq critical section helpers,
 * to be used as reference for architecture implementation.
 */

#ifndef _RSEQ_PSEUDOCODE_H
#define _RSEQ_PSEUDOCODE_H

/*
 * Pseudo-code conventions:
 *
 * rX: Register X
 * [var]: Register associated with C variable "var".
 * [label]: Jump target associated with C label "label".
 *
 * load(rX, address): load from memory address to rX
 * store(rX, address): store to memory address from rX
 * cbne(rX, rY, target): compare-and-branch to target if rX != rY
 * cbeq(rX, rY, target): compare-and-branch to target if rX == rY
 * add(rX, rY): add rY to register rX
 * memcpy(dest_address, src_address, len): copy len bytes from src_address to dst_address
 *
 * Critical section helpers identifier convention:
 * - Begin with an "rseq_" prefix,
 * - Followed by their simplified pseudo-code,
 * - Followed by __ and the type (or eventually types) on which the API
 *   applies (similar to the approach taken for C++ mangling).
 */

/*
 * rseq_load_cbne_store(v, expect, newv)
 *
 * Pseudo-code:
 *   load(r1, [v])
 *   cbne(r1, [expect], [ne])
 *   store([newv], [v])
 *
 * Return values:
 *   success: 0
 *   ne: 1
 *   abort: -1
 */

/*
 * rseq_load_add_store(v, count)
 *
 * Pseudo-code:
 *   load(r1, [v])
 *   add(r1, [count])
 *   store(r1, [v])
 *
 * Return values:
 *   success: 0
 *   abort: -1
 */

/*
 * rseq_load_cbeq_store_add_load_store(v, expectnot, voffp, load)
 *
 * Pseudo-code:
 *   load(r1, [v])
 *   cbeq(r1, [expectnot], [eq])
 *   store(r1, [load])
 *   add(r1, [voffp])
 *   load(r2, r1)
 *   store(r2, [v])
 *
 * Return values:
 *   success: 0
 *   eq: 1
 *   abort: -1
 */

/*
 * rseq_load_add_load_load_add_store(ptr, off, inc)
 *
 * Pseudo-code:
 *   load(r1, [ptr])
 *   add(r1, [off])
 *   load(r2, r1)
 *   load(r3, r2)
 *   add(r3, [inc])
 *   store(r3, r2)
 *
 * Return values:
 *   success: 0
 *   abort: -1
 */

/*
 * rseq_load_cbne_load_cbne_store(v, expect, v2, expect2, newv)
 *
 * Pseudo-code:
 *   load(r1, [v])
 *   cbne(r1, [expect], [ne])
 *   load(r2, [v2])
 *   cbne(r2, [expect2], [ne])
 *   store([newv], [v])
 *
 * Return values:
 *   success: 0
 *   ne: 1
 *   abort: -1
 */

/*
 * rseq_load_cbne_store_store(v, expect, v2, newv2, newv)
 *
 * Pseudo-code:
 *   load(r1, [v])
 *   cbne(r1, [expect], [ne])
 *   store([newv2], [v2])         // Store attempt
 *   store([newv], [v])           // Final store
 *
 * Return values:
 *   success: 0
 *   ne: 1
 *   abort: -1
 */

/*
 * rseq_load_cbne_memcpy_store(v, expect, dst, src, len, newv)
 *
 * Pseudo-code:
 *   load(r1, [v])
 *   cbne(r1, [expect], [ne])
 *   memcpy([dst], [src], [len])  // Memory copy attempt
 *   store([newv], [v])           // Final store
 *
 * Return values:
 *   success: 0
 *   ne: 1
 *   abort: -1
 */

#endif  /* _RSEQ_PSEUDOCODE_H */
