#include <userver/concurrent/striped_counter.hpp>

#include <algorithm>  // for std::max
#include <atomic>

#include <concurrent/impl/rseq.hpp>
#include <concurrent/impl/striped_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

#ifdef USERVER_IMPL_HAS_RSEQ

using NativeCounterType = std::intptr_t;

struct StripedCounter::Impl final {
  // Note that NativeCounterType is not atomic.
  //
  // All the code here is highly unportable, and for now it's x86 only.
  // For x86 the value being not-atomic works just fine.
  //
  // rseq could be unavailable, or std::thread::hardware_concurrency() could
  // also return 0 if it failed go get something meaningful from the OS, in both
  // cases we fall back to just using a std::atomic (see StripedCounter::Add).
  impl::StripedArray counters;

  std::atomic<NativeCounterType> fallback{0};
};

void StripedCounter::Add(std::uintptr_t value) noexcept {
  const auto cpu_id = rseq_cpu_start();
  if (!impl::IsCpuIdValid(cpu_id)) {
    impl_->fallback.fetch_add(value, std::memory_order_relaxed);
    return;
  }

  const auto ret = rseq_addv(RSEQ_MO_RELAXED, RSEQ_PERCPU_CPU_ID,
                             &impl_->counters[cpu_id], value, cpu_id);
  if (rseq_likely(!ret)) return;

  impl_->fallback.fetch_add(value, std::memory_order_relaxed);
}

std::uintptr_t StripedCounter::Read() const noexcept {
  auto sum = static_cast<std::uintptr_t>(
      impl_->fallback.load(std::memory_order_acquire));

  for (const auto& c : impl_->counters.Elements()) {
    // Ideally this should be a std::atomic_ref, of course
    sum += static_cast<std::uintptr_t>(__atomic_load_n(&c, __ATOMIC_ACQUIRE));
  }

  return sum;
}

#else

struct StripedCounter::Impl final {
  std::atomic<std::uintptr_t> value{0};
};

void StripedCounter::Add(std::uintptr_t value) noexcept {
  impl_->value.fetch_add(value, std::memory_order_relaxed);
}

std::uintptr_t StripedCounter::Read() const noexcept {
  return impl_->value.load(std::memory_order_acquire);
}

#endif

StripedCounter::StripedCounter() = default;

StripedCounter::~StripedCounter() = default;

std::uintptr_t StripedCounter::NonNegativeRead() const noexcept {
  return static_cast<std::uintptr_t>(
      std::max(std::intptr_t{0}, static_cast<std::intptr_t>(Read())));
}

}  // namespace concurrent

USERVER_NAMESPACE_END
