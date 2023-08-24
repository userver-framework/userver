#pragma once

#include <thread>
#include <vector>

#include <userver/concurrent/variable.hpp>
#include <userver/congestion_control/controller.hpp>
#include <userver/congestion_control/limiter.hpp>
#include <userver/congestion_control/sensor.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

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
  engine::TaskProcessor& tp_;
  std::thread thread_;
};

}  // namespace congestion_control

USERVER_NAMESPACE_END
