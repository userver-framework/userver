#include <atomic>
#include <cstdint>
#include <cstdlib>

#ifndef __clang__
#include <boost/atomic/atomic.hpp>
#endif

#include <boost/version.hpp>

struct alignas(sizeof(std::uintptr_t) * 2) A final {
  std::uintptr_t x{};
  std::uintptr_t y{};
};

#ifdef __clang__
template <typename T>
using Atomic = std::atomic<T>;
#else
template <typename T>
using Atomic = boost::atomic<T>;
#endif

extern Atomic<A> a;
Atomic<A> a{A{1, 2}};

#if defined(__GNUC__) && !defined(__clang__) && defined(__x86_64__) && \
    BOOST_VERSION < 106600
// Older Boost.Atomic fails to provide DWCAS on x86_64 GCC and goes to
// libatomic, given -mcx16. At the same time, is_lock_free always returns
// 'false' there (incorrectly). We can only hope that libatomic correctly picks
// a lock-free implementation at runtime. Last time we checked, it did.

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_DWCAS_WORKAROUND
#endif

int main() {
#ifndef USERVER_IMPL_DWCAS_WORKAROUND
  // Clang reports is_always_lock_free == true (which is correct),
  // but is_lock_free == false (which makes no sense).
  if (!a.is_always_lock_free && !a.is_lock_free()) return EXIT_FAILURE;
#endif

  A expected{1, 2};
  A desired{3, 4};
  if (!a.compare_exchange_strong(expected, desired)) return EXIT_FAILURE;

  expected = {3, 4};
  desired = {5, 6};
  if (!a.compare_exchange_strong(expected, desired)) return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
