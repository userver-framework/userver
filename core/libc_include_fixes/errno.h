#pragma once

// This file is required for correct errno behavior in optimized code
// with context continuation in an another thread (at least for glibc and musl).
// It removes `__attribute__((const))` from __errno_location().
//
// Based on phantom, kudos to mamchits@ for the solution and
// to dzhuk@ for pointing this out.

#define __errno_location __errno_location_wrong_prototype
#include_next <errno.h>
#undef __errno_location

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __THROW
#define USERVER_IMPL_LIBC_INCLUDE_FIXES_THROW __THROW
#else
#define USERVER_IMPL_LIBC_INCLUDE_FIXES_THROW
#endif

extern int* __errno_location(void) USERVER_IMPL_LIBC_INCLUDE_FIXES_THROW;

#undef USERVER_IMPL_LIBC_INCLUDE_FIXES_THROW

#ifdef __cplusplus
}
#endif
