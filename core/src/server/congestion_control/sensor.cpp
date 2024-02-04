#include <userver/server/congestion_control/sensor.hpp>

#include <engine/task/task_processor.hpp>
#include <server/net/stats.hpp>
#include <userver/server/server.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::congestion_control {

namespace {
const std::chrono::seconds kSecond{1};
}

Sensor::Sensor(engine::TaskProcessor& tp) : tp_(tp) {}

void Sensor::RegisterRequestsSource(RequestsSource& source) {
  auto requests_sources = requests_sources_.StartWrite();
  requests_sources->push_back(&source);
  requests_sources.Commit();
}

Sensor::Data Sensor::FetchCurrent() {
  const bool first_fetch =
      last_fetch_tp_ == std::chrono::steady_clock::time_point{};
  auto now = std::chrono::steady_clock::now();
  auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - last_fetch_tp_);

  // Seems impossible, but has to re-check
  if (duration_ms.count() == 0) duration_ms = std::chrono::milliseconds(1);

  auto overloads = tp_.GetTaskCounter().GetTasksOverloadSensor().value;
  auto overloads_ps = (overloads - last_overloads_) * kSecond / duration_ms;

  auto no_overloads = tp_.GetTaskCounter().GetTasksNoOverloadSensor().value;
  auto no_overloads_ps =
      (no_overloads - last_no_overloads_) * kSecond / duration_ms;

  std::uint64_t requests{0};
  auto requests_sources = requests_sources_.Read();
  for (const auto& source : *requests_sources)
    requests += source->GetTotalRequests();
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
