#pragma once

#include <cxxabi.h>
#include <cstring>

#ifdef _LIBCPP_VERSION

struct EhGlobals {};

#elif defined(__GLIBCXX__)
// This structure is a __cxa_eh_globals storage for coroutines.
// It is required to allow task switching during unwind.
struct EhGlobals {
  void* data[4];
  EhGlobals() { ::memset(data, 0, sizeof(data)); }
};
#else
#error "We don't support stack unwinding with your c++ stdlib implementation"
#endif
