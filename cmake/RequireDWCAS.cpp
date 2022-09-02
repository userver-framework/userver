#include <atomic>
#include <cstdint>

#ifdef USERVER_USE_BOOST_DWCAS
#include <boost/atomic/atomic.hpp>
#endif

struct alignas(sizeof(std::uintptr_t) * 2) A final {
  std::uintptr_t x{};
  std::uintptr_t y{};
};

#ifdef USERVER_USE_BOOST_DWCAS
template <typename T>
using Atomic = boost::atomic<T>;
#else
template <typename T>
using Atomic = std::atomic<T>;
#endif

extern Atomic<A> a;
Atomic<A> a{A{1, 2}};

int main() {
  // Clang reports is_always_lock_free == true (which is correct),
  // but is_lock_free == false (which makes no sense).
  if (!Atomic<A>::is_always_lock_free && !a.is_lock_free()) return 1;

  A expected{1, 2};
  A desired{3, 4};
  if (!a.compare_exchange_strong(expected, desired)) return 2;

  expected = {3, 4};
  desired = {5, 6};
  if (!a.compare_exchange_strong(expected, desired)) return 2;

  return 0;
}
