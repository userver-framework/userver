#pragma once

#include <thread>
#include <vector>

#include <concurrent/variable.hpp>
#include <congestion_control/controller.hpp>
#include <congestion_control/limiter.hpp>
#include <congestion_control/sensor.hpp>

namespace congestion_control {

/// A Watchdog that pings registered sensors and updates limits.
/// @note It spawns a separate non-coroutine thread to be more simple and
/// robust.
class Watchdog final {
 public:
  Watchdog();

  ~Watchdog();

  void Register(ControllerInfo ci);

  void Stop();

 private:
  void Check();

  concurrent::Variable<std::vector<ControllerInfo>, std::mutex> cis_;
  std::atomic<bool> should_stop_;
  std::thread thread_;
};

}  // namespace congestion_control
