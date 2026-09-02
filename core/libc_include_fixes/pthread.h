#pragma once

// This file is required for correct pthread_self behavior in optimized code
// with context continuation in an another thread (at least for glibc and musl).
// It removes `__attribute__((const))` from pthread_self().
//
// Based on phantom, kudos to mamchits@ for the solution and
// to dzhuk@ for pointing this out.

#define pthread_self pthread_self_wrong_prototype
#include_next <pthread.h>
#undef pthread_self

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __THROW
#define USERVER_IMPL_LIBC_INCLUDE_FIXES_THROW __THROW
#else
#define USERVER_IMPL_LIBC_INCLUDE_FIXES_THROW
#endif

extern pthread_t pthread_self(void) USERVER_IMPL_LIBC_INCLUDE_FIXES_THROW;

#undef USERVER_IMPL_LIBC_INCLUDE_FIXES_THROW

#ifdef __cplusplus
}
#endif
