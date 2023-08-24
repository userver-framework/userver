#include <userver/congestion_control/controllers/linear_config.hpp>

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

Config::Config(formats::json::Value config) {
  errors_threshold_percent = config["errors-threshold-percent"].As<double>(5.0);
  safe_delta_limit = config["deactivate-delta"].As<std::size_t>(10);
  timings_burst_threshold =
      config["timings-burst-times-threshold"].As<double>(5);
  min_timings =
      std::chrono::milliseconds(config["min-timings-ms"].As<std::size_t>(20));
  min_limit = config["min-limit"].As<std::size_t>(10);
  min_qps = config["min-qps"].As<std::size_t>(10);
}

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
