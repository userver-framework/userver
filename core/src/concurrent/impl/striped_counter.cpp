#include <concurrent/impl/striped_counter.hpp>

#include <atomic>

#if defined(__x86_64__) && defined(__linux__) && \
    !defined(USERVER_DISABLE_RSEQ_ACCELERATION)
#define HAS_RSEQ
#endif

#ifdef HAS_RSEQ
#include <rseq/rseq.h>

#include <thread>

#include <concurrent/impl/interference_shield.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

namespace {

using NativeCounterType = std::intptr_t;

struct RseqThreadInit final {
  RseqThreadInit() : is_rseq_available{rseq_register_current_thread() == 0} {}
  ~RseqThreadInit() { rseq_unregister_current_thread(); }

  const bool is_rseq_available;
};

[[maybe_unused]] thread_local RseqThreadInit rseq_initializer{};

std::size_t CalculateCountersArraySize() {
  // Due to compiler caching thread_local access, us switching fibers between
  // threads and all that jazz, we could read this field from another thread's
  // initializer.
  // However, we assume that 'rseq_register_current_thread' either succeeds or
  // fails for every thread, so this is fine, and for all we care
  // 'rseq_register_current_thread' was called for the current thread at some
  // point.
  if (!rseq_initializer.is_rseq_available) {
    return 0;
  }

  return std::thread::hardware_concurrency();
}

}  // namespace

struct StripedCounter::Impl final {
  // Note that NativeCounterType is not atomic.
  //
  // All the code here is highly unportable, and for now it's x86 only.
  // For x86 the value being not-atomic works just fine.
  //
  // rseq could be unavailable, or std::thread::hardware_concurrency() could
  // also return 0 if it failed go get something meaningful from the OS, in both
  // cases we fall back to just using a std::atomic (see StripedCounter::Add).
  utils::FixedArray<InterferenceShield<NativeCounterType>> counters{
      CalculateCountersArraySize(), 0};

  std::atomic<NativeCounterType> fallback{0};
};

StripedCounter::StripedCounter() = default;

StripedCounter::~StripedCounter() = default;

void StripedCounter::Add(std::uintptr_t value) noexcept {
  const auto cpu_id = rseq_cpu_start();
  if (rseq_unlikely(cpu_id >= impl_->counters.size())) {
    UASSERT_MSG(
        impl_->counters.empty(),
        "CPU count is not properly detected. Please report this issue."
        "You may want to set USERVER_DISABLE_RSEQ_ACCELERATION=ON cmake "
        "variable to overcome this error.");
    impl_->fallback.fetch_add(value, std::memory_order_relaxed);
    return;
  }

  if (rseq_addv(RSEQ_MO_RELAXED, RSEQ_PERCPU_CPU_ID,  // indentation
                &*impl_->counters[cpu_id], value, cpu_id)) {
    impl_->fallback.fetch_add(value, std::memory_order_relaxed);
  }
}

std::uint64_t StripedCounter::Read() const noexcept {
  std::uintptr_t sum{static_cast<std::uintptr_t>(
      impl_->fallback.load(std::memory_order_relaxed))};
  for (const auto& c : impl_->counters) {
    // Ideally this should be a std::atomic_ref, of course
    sum += __atomic_load_n(&*c, __ATOMIC_RELAXED);
  }

  return sum;
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
#undef HAS_RSEQ
#else

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

struct StripedCounter::Impl final {
  std::atomic<std::uint64_t> value{0};
};

StripedCounter::StripedCounter() = default;
StripedCounter::~StripedCounter() = default;

void StripedCounter::Add(std::uintptr_t value) noexcept {
  impl_->value.fetch_add(value, std::memory_order_relaxed);
}

std::uint64_t StripedCounter::Read() const noexcept {
  return impl_->value.load(std::memory_order_relaxed);
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END

#endif
