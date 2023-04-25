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

std::string Sensor::Data::ToLogString() {
  return fmt::format("total={} events={} current_load={}", total, timeouts,
                     current_load);
}

}  // namespace v2
}  // namespace congestion_control

USERVER_NAMESPACE_END
