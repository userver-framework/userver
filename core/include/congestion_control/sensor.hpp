#pragma once

#include <chrono>

namespace congestion_control {

/// Abstract sensor that fetches counters. These counters will be used
/// to forecast saturation levels and limit current load.
class Sensor {
 public:
  struct Data {
    size_t current_load;
    size_t overload_events_count;
    std::chrono::steady_clock::time_point tp;
  };

  virtual ~Sensor() = default;

  /// @brief Fetch current counters
  /// @note Can be called both from coroutine and non-coroutine context
  virtual Data FetchCurrent() = 0;
};

}  // namespace congestion_control
