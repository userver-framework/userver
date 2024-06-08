#pragma once

/// @file userver/utils/statistics/striped_rate_counter.hpp
/// @brief @copybrief utils::statistics::StripedRateCounter

#include <atomic>
#include <cstdint>

#include <userver/concurrent/striped_counter.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/rate.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Atomic counter of type Rate with relaxed memory ordering, increased
/// memory consumption and decreased contention.
///
/// This class is represented as Rate metric when serializing to statistics.
///
/// Differences from utils::statistics::RateCounter:
//
/// 1. StripedRateCounter takes `8 * N_CORES` memory, while RateCounter is just
///    a single atomic.
/// 2. StripedRateCounter uses split counters on x86_64, which results
///    in perfect performance even under heavy concurrent usage.
///
/// Use StripedRateCounter instead of RateCounter sparingly, in places where a
/// lot of threads are supposed to be hammering on the same metrics.
class StripedRateCounter final {
 public:
  using ValueType = Rate;

  StripedRateCounter() = default;

  explicit StripedRateCounter(Rate desired) { Add(desired); }

  explicit StripedRateCounter(Rate::ValueType desired)
      : StripedRateCounter(Rate{desired}) {}

  StripedRateCounter(const StripedRateCounter& other) {
    val_.Add(other.LoadImpl());
  }

  StripedRateCounter& operator=(const StripedRateCounter& other) noexcept {
    if (this == &other) return *this;

    offset_ = other.LoadImpl() - val_.Read();
    return *this;
  }

  StripedRateCounter& operator=(Rate desired) noexcept {
    Store(desired);
    return *this;
  }

  void Store(Rate desired) noexcept {
    offset_ = static_cast<std::uintptr_t>(desired.value) - val_.Read();
  }

  Rate Load() const noexcept {
    return Rate{static_cast<std::uint64_t>(LoadImpl())};
  }

  void Add(Rate arg) noexcept {
    val_.Add(static_cast<std::uintptr_t>(arg.value));
  }

  StripedRateCounter& operator++() noexcept {
    val_.Add(1);
    return *this;
  }

  StripedRateCounter& operator+=(Rate arg) noexcept {
    Add(arg);
    return *this;
  }

  StripedRateCounter& operator+=(const StripedRateCounter& arg) noexcept {
    val_.Add(arg.LoadImpl());
    return *this;
  }

 private:
  // Implementation note: any operation that assigns a value (instead of adding)
  // would induce race condition if implemented using LoadImpl(). Use offset_
  // directly instead for those.
  std::uintptr_t LoadImpl() const noexcept {
    return val_.Read() + offset_.load(std::memory_order_relaxed);
  }

  concurrent::StripedCounter val_;
  std::atomic<std::uintptr_t> offset_{0};
};

void DumpMetric(Writer& writer, const StripedRateCounter& value);

void ResetMetric(StripedRateCounter& value);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
