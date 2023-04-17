#include <userver/rcu/read_indicator.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <userver/utils/assert.hpp>

#include <rcu/concurrent_deque.hpp>
#include <rcu/thread_id.hpp>

USERVER_NAMESPACE_BEGIN

namespace rcu {

namespace {

struct Counter final {
  std::atomic<std::uint64_t> acquired{0};
  std::atomic<std::uint64_t> released{0};
};

void IncrementRelaxed(std::atomic<std::uint64_t>& counter) noexcept {
  counter.store(counter.load(std::memory_order_relaxed) + 1,
                std::memory_order_relaxed);
}

using CounterArray = impl::ConcurrentDeque<Counter, 8, 17>;
static_assert(CounterArray::kMaxSize == impl::kMaxThreadCount);

Counter& GetLocalCounter(CounterArray& counters) noexcept {
  thread_local CounterArray::FastIndex index{impl::GetCurrentThreadId()};
  return counters[index];
}

void Acquire(CounterArray& counters) noexcept {
  IncrementRelaxed(GetLocalCounter(counters).acquired);
}

void Release(CounterArray& counters) noexcept {
  IncrementRelaxed(GetLocalCounter(counters).released);
}

}  // namespace

struct ReadIndicator::Impl final {
  CounterArray counters{impl::GetThreadCount()};
  std::uint64_t previous_total_released{0};
};

ReadIndicator::ReadIndicator() noexcept : impl_() {}

ReadIndicator::~ReadIndicator() {
  UASSERT_MSG(IsFree(), "ReadIndicator is destroyed while being used");
}

ReadIndicatorLock ReadIndicator::Lock() noexcept {
  Acquire(impl_->counters);
  return ReadIndicatorLock{*this};
}

bool ReadIndicator::IsFree() noexcept {
  // The following situation can happen:
  // 1. CountTotal begins traversing the counters. Let's say it's currently at
  //    counter #20
  // 2. Thread #10 copies the lock owned by thread #30
  // 3. Thread #30 releases the lock
  // 4. CountTotal finished counting and returns. It doesn't account for the
  //    lock #10 and reports that the ReadIndicator is free
  //
  // Alternatively, instead of thread #10, a new thread could be created, which
  // would also not be accounted for by the current CountTotal call.
  //
  // To behave correctly in those cases, we must walk the counters twice and
  // only return 'true' if no new locks have been released.
  for (int times = 0; times < 2; ++times) {
    std::uint64_t total_acquired = 0;
    std::uint64_t total_released = 0;
    impl_->counters.Walk([&](Counter& counter) {
      total_acquired += counter.acquired.load(std::memory_order_relaxed);
      total_released += counter.released.load(std::memory_order_relaxed);
    });

    const auto previous_total_released =
        std::exchange(impl_->previous_total_released, total_released);
    if (total_acquired != total_released) return false;
    if (total_released == previous_total_released) return true;
  }
  return false;
}

ReadIndicatorLock::ReadIndicatorLock(ReadIndicator& indicator) noexcept
    : indicator_(&indicator) {}

ReadIndicatorLock::ReadIndicatorLock(ReadIndicatorLock&& other) noexcept
    : indicator_(std::exchange(other.indicator_, nullptr)) {}

ReadIndicatorLock::ReadIndicatorLock(const ReadIndicatorLock& other) noexcept
    : indicator_(other.indicator_) {
  if (indicator_ != nullptr) Acquire(indicator_->impl_->counters);
}

ReadIndicatorLock& ReadIndicatorLock::operator=(
    ReadIndicatorLock&& other) noexcept {
  if (this != &other) {
    if (indicator_ != nullptr) Release(indicator_->impl_->counters);
    indicator_ = std::exchange(other.indicator_, nullptr);
  }
  return *this;
}

ReadIndicatorLock& ReadIndicatorLock::operator=(
    const ReadIndicatorLock& other) noexcept {
  *this = ReadIndicatorLock{other};
  return *this;
}

ReadIndicatorLock::~ReadIndicatorLock() {
  if (indicator_ != nullptr) Release(indicator_->impl_->counters);
}

}  // namespace rcu

USERVER_NAMESPACE_END
