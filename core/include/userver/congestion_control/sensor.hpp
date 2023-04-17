#pragma once

#include <chrono>
#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace congestion_control {

/// Abstract sensor that fetches counters. These counters will be used
/// to forecast saturation levels and limit current load.
class Sensor {
 public:
  struct Data {
    std::uint64_t current_load{0};
    std::uint64_t overload_events_count{0};
    std::uint64_t no_overload_events_count{0};
    std::chrono::steady_clock::time_point tp;

    double GetLoadPercent() const;
  };

  virtual ~Sensor() = default;

  /// @brief Fetch current counters
  /// @note Can be called both from coroutine and non-coroutine context
  virtual Data FetchCurrent() = 0;
};

namespace v2 {

class Sensor {
 public:
  struct Data {
    std::size_t total{0};
    std::size_t timeouts{0};

    std::size_t current_limit{0};

    double GetRate() const {
      return static_cast<double>(timeouts) / (total ? total : 1);
    }

    void operator+=(const Data& other) {
      total += other.total;
      timeouts += other.timeouts;
    }
  };

  virtual ~Sensor() = default;

  virtual Data GetCurrent() = 0;
};

}  // namespace v2

}  // namespace congestion_control

USERVER_NAMESPACE_END
