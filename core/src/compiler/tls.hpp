#pragma once

// This attribute should be put on functions that read or write (or read the
// address of) 'thread_local' variables. (And such functions should never
// contain a context switch themselves.) It prevents the compiler from caching
// the TLS address across a coroutine context switch.
#define USERVER_PREVENT_TLS_CACHING __attribute__((noinline))
