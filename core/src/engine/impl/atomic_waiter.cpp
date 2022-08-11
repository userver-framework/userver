#include <engine/impl/atomic_waiter.hpp>

#include <type_traits>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {
auto* const kInvalidTaskContextPtr = reinterpret_cast<TaskContext*>(1);
}  // namespace

// We use boost::atomic, because std::atomic refuses to produce double-width
// compare-and-swap instruction (DWCAS) under x86_64 on some compilers.
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80878
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=84522

// The type used in boost::atomic must have no padding to perform CAS safely.
static_assert(std::has_unique_object_representations_v<Waiter>);

AtomicWaiter::AtomicWaiter() noexcept : waiter_(Waiter{}) {
  UASSERT(waiter_.is_lock_free());
}

// Аll double-width atomic operations other than CAS are typically implemented
// in terms of the DWCAS instruction. But in the case of Waiter, we can
// implement them more efficiently, so we do that instead of using 'load',
// 'store' and 'exchange'.

bool AtomicWaiter::IsEmptyRelaxed() noexcept {
  Waiter expected{kInvalidTaskContextPtr, 0};
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
  UASSERT_MSG(
      success,
      fmt::format("Attempting to wait in a single AtomicWaiter "
                  "from multiple coroutines: new=({}, {}) existing=({}, {})",
                  fmt::ptr(new_value.context), new_value.epoch,
                  fmt::ptr(expected.context), expected.epoch));
}

bool AtomicWaiter::ResetIfEquals(Waiter expected) noexcept {
  const auto old_expected = expected;
  const bool success = waiter_.compare_exchange_strong(
      expected, Waiter{}, boost::memory_order_release,
      boost::memory_order_relaxed);
  UASSERT_MSG(success || !expected.context,
              fmt::format("An unexpected context is occupying the "
                          "AtomicWaiter: expected=({}, {}) actual=({}, {})",
                          fmt::ptr(old_expected.context), old_expected.epoch,
                          fmt::ptr(expected.context), expected.epoch));
  return success;
}

Waiter AtomicWaiter::GetAndReset() noexcept {
  // Optimize for the common case of empty Waiter.
  Waiter expected{};
  while (true) {
    const bool success = waiter_.compare_exchange_weak(
        expected, Waiter{}, boost::memory_order_acq_rel,
        boost::memory_order_acquire);
    if (success || !expected.context) break;
  }
  return expected;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
