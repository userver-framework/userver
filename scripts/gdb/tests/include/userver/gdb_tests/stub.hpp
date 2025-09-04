#pragma once

#include <atomic>

//  NOLINTBEGIN

template <class Tp>
inline __attribute__((always_inline)) void DoNotOptimize(Tp&& value) {
#if defined(__clang__)
    asm volatile("" : "+r,m"(value) : : "memory");
#else
    asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

static char* volatile mark;

__attribute__((noinline)) void DoSomeStuff() {
    std::atomic_signal_fence(std::memory_order_acq_rel);
    DoNotOptimize(mark);
    std::atomic_signal_fence(std::memory_order_acq_rel);
}

#define TEST_INIT(value) DoSomeStuff()

#define TEST_EXPR(value, expected) DoSomeStuff()

#define TEST_DEINIT(value) DoSomeStuff()

#define TEST_COMMAND(...) DoSomeStuff()

#define MAKE_COREDUMP_AND_SWITCH_TO() DoSomeStuff()

//  NOLINTEND
