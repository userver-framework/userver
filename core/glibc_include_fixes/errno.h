#pragma once

// This file is required for correct errno behavior with thread switching.
// It removes `const` attribute from __errno_location().
//
// Based on phantom, kudos to mamchits@ for the solution and
// to dzhuk@ for pointing this out.

#ifdef __GLIBCXX__

#define __errno_location __errno_location_wrong_prototype
#include_next <errno.h>
#undef __errno_location

__BEGIN_DECLS

extern int* __errno_location(void) __THROW;

__END_DECLS

#else  // defined(__GLIBCXX__)

#include_next <errno.h>

#endif  // defined(__GLIBCXX__)
