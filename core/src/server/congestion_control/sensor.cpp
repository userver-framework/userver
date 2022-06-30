#include <server/congestion_control/sensor.hpp>

#include <engine/task/task_processor.hpp>
#include <server/net/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::congestion_control {

namespace {
const std::chrono::milliseconds kSecond{1000};
}

Sensor::Sensor(const Server& server, engine::TaskProcessor& tp)
    : server_(server), tp_(tp) {}

Sensor::Data Sensor::FetchCurrent() {
  bool first_fetch = last_fetch_tp_ == std::chrono::steady_clock::time_point{};
  auto now = std::chrono::steady_clock::now();
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - last_fetch_tp_);

  // Seems impossible, but has to re-check
  if (duration_ms.count() == 0) duration_ms = std::chrono::milliseconds(1);

  auto overloads = tp_.GetTaskCounter().GetTasksOverloadSensor();
  auto overloads_ps = (overloads - last_overloads_) * kSecond / duration_ms;

  auto no_overloads = tp_.GetTaskCounter().GetTasksNoOverloadSensor();
  auto no_overloads_ps =
      (no_overloads - last_no_overloads_) * kSecond / duration_ms;

  // TODO: wrong value, it includes ratelimited ones too
  //       it might lead to too high start RPS limits
  auto server_stats = server_.GetServerStats();
  auto requests = server_stats.active_request_count.load() +
                  server_stats.requests_processed_count.load();
  auto rps = (requests - last_requests_) * kSecond / duration_ms;

  last_fetch_tp_ = now;
  last_overloads_ = overloads;
  last_no_overloads_ = no_overloads;
  last_requests_ = requests;

  return Data{
      first_fetch ? 0 : rps,
      first_fetch ? 0 : overloads_ps,
      first_fetch ? 0 : no_overloads_ps,
      now,
  };
}

}  // namespace server::congestion_control

USERVER_NAMESPACE_END
