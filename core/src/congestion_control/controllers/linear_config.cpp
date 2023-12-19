#include <userver/congestion_control/controllers/linear_config.hpp>

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

Config Parse(const formats::json::Value& value, formats::parse::To<Config>) {
  Config result;
  result.errors_threshold_percent =
      value["errors-threshold-percent"].As<double>(5.0);
  result.safe_delta_limit = value["deactivate-delta"].As<std::size_t>(10);
  result.timings_burst_threshold =
      value["timings-burst-times-threshold"].As<double>(5);
  result.min_timings =
      std::chrono::milliseconds(value["min-timings-ms"].As<std::size_t>(20));
  result.min_limit = value["min-limit"].As<std::size_t>(10);
  result.min_qps = value["min-qps"].As<std::size_t>(10);
  return result;
}

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
