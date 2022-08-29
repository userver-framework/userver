#include <cstdint>
#include <cstdlib>

#include <boost/atomic/atomic.hpp>
#include <boost/version.hpp>

struct alignas(sizeof(std::uintptr_t) * 2) A final {
  std::uintptr_t x{};
  std::uintptr_t y{};
};

extern boost::atomic<A> a;
boost::atomic<A> a{A{1, 2}};

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
  if (!a.is_lock_free()) return EXIT_FAILURE;
#endif

  A expected{1, 2};
  A desired{3, 4};
  if (!a.compare_exchange_strong(expected, desired)) return EXIT_FAILURE;

  expected = {3, 4};
  desired = {5, 6};
  if (!a.compare_exchange_strong(expected, desired)) return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
