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

extern pthread_t pthread_self(void)
#ifdef __THROW
    __THROW
#endif
    ;

#ifdef __cplusplus
}
#endif
