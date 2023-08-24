#include <userver/congestion_control/sensor.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace congestion_control {

double Sensor::Data::GetLoadPercent() const {
  auto total = overload_events_count + no_overload_events_count;
  if (total != 0)
    return static_cast<double>(overload_events_count * 100) / total;
  else
    return 0;
}

namespace v2 {

std::string Sensor::Data::ToLogString() const {
  return fmt::format("events={}/{} timings_avg={}ms current_load={}", timeouts,
                     total, timings_avg_ms, current_load);
}

}  // namespace v2
}  // namespace congestion_control

USERVER_NAMESPACE_END
