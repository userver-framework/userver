#pragma once

// This file is required for correct pthread_self behavior with thread switches.
// It removes `const` attribute from pthread_self().
//
// Based on phantom, kudos to mamchits@ for the solution and
// to dzhuk@ for pointing this out.

#ifdef __GLIBCXX__

#define pthread_self pthread_self_wrong_prototype
#include_next <pthread.h>
#undef pthread_self

__BEGIN_DECLS

extern pthread_t pthread_self(void) __THROW;

__END_DECLS

#else  // defined(__GLIBCXX__)

#include_next <pthread.h>

#endif  // defined(__GLIBCXX__)
