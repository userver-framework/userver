#include <engine/impl/atomic_waiter.hpp>

#include <type_traits>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

static_assert(std::has_unique_object_representations_v<Waiter>);  // for CAS
static_assert(boost::atomic<Waiter>::is_always_lock_free);

// We use boost::atomic, because refuses to produce double-width
// compare-and-swap instructions (DWCAS) under x86_64 on some compilers.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84522
//
// Furthermore, all double-width atomic operations other than CAS are typically
// implemented in terms of the DWCAS instruction. But in the case of Waiter, we
// can implement them more efficiently, so we do that instead of using 'load',
// 'store' and 'exchange'.

bool AtomicWaiter::IsEmpty() noexcept {
  Waiter expected{reinterpret_cast<TaskContext*>(1), 0};
  const bool success = waiter_.compare_exchange_strong(
      expected, expected, boost::memory_order_relaxed,
      boost::memory_order_relaxed);
  UASSERT(!success);
  return expected.context == nullptr;
}

void AtomicWaiter::Set(Waiter new_value) noexcept {
  Waiter expected{};
  // seq_cst is important for the "Append-Check-Wakeup" sequence.
  const bool success = waiter_.compare_exchange_strong(
      expected, new_value, boost::memory_order_seq_cst,
      boost::memory_order_relaxed);
  UASSERT_MSG(success,
              "Attempting to wait in a single AtomicWaiter "
              "from multiple coroutines");
}

Waiter AtomicWaiter::GetAndReset() noexcept {
  // The initial CAS always fails. This is done to reduce contention for the
  // common case of AtomicWaiter being empty.
  Waiter expected{reinterpret_cast<TaskContext*>(1), 0};
  while (true) {
    const bool success = waiter_.compare_exchange_strong(
        expected, Waiter{}, boost::memory_order_acq_rel,
        boost::memory_order_acquire);
    if (success | !expected.context) break;
  }
  return expected;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
