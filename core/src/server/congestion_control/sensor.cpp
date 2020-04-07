#include <server/congestion_control/sensor.hpp>

#include <engine/task/task_processor.hpp>
#include <server/net/stats.hpp>

namespace server::congestion_control {

Sensor::Sensor(const Server& server, engine::TaskProcessor& tp)
    : server_(server), tp_(tp), last_overloads_(0), last_requests_(0) {}

Sensor::Data Sensor::FetchCurrent() {
  bool first_fetch = last_fetch_tp_ == std::chrono::steady_clock::time_point{};
  auto now = std::chrono::steady_clock::now();
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - last_fetch_tp_);

  // Seems impossible, but has to re-check
  if (duration_ms.count() == 0) duration_ms = std::chrono::milliseconds(1);

  auto overloads = tp_.GetTaskCounter().GetTasksOverloadSensor();
  auto overloads_ps = (overloads - last_overloads_) *
                      std::chrono::milliseconds(1000) / duration_ms;

  // TODO: wrong value, it includes ratelimited ones too
  auto server_stats = server_.GetServerStats();
  auto requests = server_stats.active_request_count.load() +
                  server_stats.requests_processed_count.load();
  auto rps = (requests - last_requests_) * std::chrono::milliseconds(1000) /
             duration_ms;

  last_fetch_tp_ = now;
  last_overloads_ = overloads;
  last_requests_ = requests;

  return Data{
      first_fetch ? 0 : rps,
      first_fetch ? 0 : overloads_ps,
      now,
  };
}

}  // namespace server::congestion_control
