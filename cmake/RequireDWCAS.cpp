#include <cstdint>
#include <cstdlib>

#include <boost/atomic/atomic.hpp>

struct alignas(sizeof(std::uintptr_t) * 2) A final {
  std::uintptr_t x{};
  std::uintptr_t y{};
};

extern boost::atomic<A> a;
boost::atomic<A> a{A{1, 2}};

int main() {
  if (!a.is_lock_free()) return EXIT_FAILURE;

  A expected{1, 2};
  A desired{3, 4};
  if (!a.compare_exchange_strong(expected, desired)) return EXIT_FAILURE;

  expected = {3, 4};
  desired = {5, 6};
  if (!a.compare_exchange_strong(expected, desired)) return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
