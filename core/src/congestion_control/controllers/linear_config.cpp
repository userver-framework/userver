#include <userver/congestion_control/controllers/linear_config.hpp>

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

Config Parse(const formats::json::Value& value, formats::parse::To<Config>) {
    return {
        .errors_threshold_percent = value["errors-threshold-percent"].As<double>(5.0),
        .safe_delta_limit = value["deactivate-delta"].As<std::size_t>(10),
        .timings_burst_threshold = static_cast<std::size_t>(value["timings-burst-times-threshold"].As<double>(5)),
        .min_timings = std::chrono::milliseconds(value["min-timings-ms"].As<std::size_t>(20)),
        .min_limit = value["min-limit"].As<std::size_t>(10),
        .min_qps = value["min-qps"].As<std::size_t>(10),
        .use_separate_stats = value["use-separate-stats"].As<bool>(false),
    };
}

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
