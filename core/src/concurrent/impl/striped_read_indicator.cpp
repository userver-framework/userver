#include <userver/concurrent/impl/striped_read_indicator.hpp>

#include <atomic>
#include <limits>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

StripedReadIndicator::StripedReadIndicator() = default;

StripedReadIndicator::~StripedReadIndicator() {
  UASSERT_MSG(IsFree(), "ReadIndicator is destroyed while being used");
}

StripedReadIndicatorLock StripedReadIndicator::Lock() noexcept {
  DoLock();
  return StripedReadIndicatorLock{*this};
}

bool StripedReadIndicator::IsFree() const noexcept {
  return GetActiveCountApprox() == 0;
}

std::uintptr_t StripedReadIndicator::GetAcquireCountApprox() const noexcept {
  return acquired_count_.Read();
}

std::uintptr_t StripedReadIndicator::GetReleaseCountApprox() const noexcept {
  return released_count_.Read();
}

std::uintptr_t StripedReadIndicator::GetActiveCountApprox() const noexcept {
  // It's important that for every lock operation, if released_count_.Add(1)
  // is seen by an IsFree() call, then the preceding acquired_count_.Add(1)
  // is also seen by it. Otherwise IsFree() could falsely report 'true'.
  //
  // Proof that it holds. According to the C++ standard, [atomics.fences],
  // release fence (A) synchronizes-with atomic operation (B), because:
  // 1. (B) performs acquire on every atomic counter within 'released_count_',
  //    including the counter (M) touched by some DoUnlock call
  // 2. (A) is sequenced before atomic operation (X)
  // 3. (X) modifies (M)
  // 4. (B) reads the value written by (X)
  // Thus if (B) observes (X), then it observes all stores sequenced
  // before (A), including the preceding acquired_count_.Add(1).
  //
  // The same proof holds for when a lock is copied, then the old lock is
  // destroyed.
  //
  // This guarantee cannot be achieved by a single striped counter. Example:
  // 0. counter=[0,1]
  // 1. [writer] start IsFree, read counter[0]==0
  // 2. [reader, cpu #0] DoLock, counter=[1,1]
  // 3. [reader, cpu #1] DoUnlock, counter=[1,0]
  // 4. [writer] continue IsFree, read counter[1]==0 => IsFree()==true
  // Even with all seq_cst operations, the case above holds.
  const auto released = released_count_.Read();  // (B)
  const auto acquired = acquired_count_.Read();

  // Check that acquired >= released (modulo the overflow)
  // See the explanation above for why this should hold.
  UASSERT(acquired - released <=
          std::numeric_limits<std::uintptr_t>::max() / 2);

  return acquired - released;
}

void StripedReadIndicator::DoLock() noexcept { acquired_count_.Add(1); }

void StripedReadIndicator::DoUnlock() noexcept {
  std::atomic_thread_fence(std::memory_order_release);  // (A)
  released_count_.Add(1);                               // (X)
}

StripedReadIndicatorLock::StripedReadIndicatorLock(
    StripedReadIndicator& indicator) noexcept
    : indicator_(&indicator) {}

StripedReadIndicatorLock::StripedReadIndicatorLock(
    StripedReadIndicatorLock&& other) noexcept
    : indicator_(std::exchange(other.indicator_, nullptr)) {}

StripedReadIndicatorLock::StripedReadIndicatorLock(
    const StripedReadIndicatorLock& other) noexcept
    : indicator_(other.indicator_) {
  if (indicator_ != nullptr) {
    indicator_->DoLock();
  }
}

StripedReadIndicatorLock& StripedReadIndicatorLock::operator=(
    StripedReadIndicatorLock&& other) noexcept {
  if (this != &other) {
    if (indicator_ != nullptr) {
      indicator_->DoUnlock();
    }
    indicator_ = std::exchange(other.indicator_, nullptr);
  }
  return *this;
}

StripedReadIndicatorLock& StripedReadIndicatorLock::operator=(
    const StripedReadIndicatorLock& other) noexcept {
  *this = StripedReadIndicatorLock{other};
  return *this;
}

StripedReadIndicatorLock::~StripedReadIndicatorLock() {
  if (indicator_ != nullptr) {
    indicator_->DoUnlock();
  }
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
