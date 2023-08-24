#include <userver/congestion_control/config.hpp>

#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control {

namespace {

template <typename T>
T ParsePercent(const formats::json::Value& json) {
  const auto value = json.As<T>();
  if (value < 0 || value > 100) {
    throw std::runtime_error(
        fmt::format("Validation 0 <= x <= 100 failed for '{}' (got: {})",
                    json.GetPath(), value));
  }
  return value;
}

template <typename T>
T ParseNonNegative(const formats::json::Value& json) {
  const auto value = json.As<T>();
  if (value < 0) {
    throw std::runtime_error(fmt::format(
        "Validation 0 <= x failed for '{}' (got: {})", json.GetPath(), value));
  }
  return value;
}

}  // namespace

Policy Parse(const formats::json::Value& policy, formats::parse::To<Policy>) {
  Policy p;
  p.min_limit = ParseNonNegative<int>(policy["min-limit"]);
  p.up_rate_percent = ParsePercent<double>(policy["up-rate-percent"]);
  p.down_rate_percent = ParsePercent<double>(policy["down-rate-percent"]);
  p.overload_on = policy["overload-on-seconds"].As<int>();
  p.overload_off = policy["overload-off-seconds"].As<int>();
  p.up_count = policy["up-level"].As<int>();
  p.down_count = policy["down-level"].As<int>();
  p.no_limit_count = policy["no-limit-seconds"].As<int>();
  p.load_limit_percent = policy["load-limit-percent"].As<int>(0);
  p.load_limit_crit_percent = policy["load-limit-crit-percent"].As<int>(101);
  p.start_limit_factor = policy["start-limit-factor"].As<double>(0.75);
  return p;
}

namespace impl {

RpsCcConfig RpsCcConfig::Parse(const dynamic_config::DocsMap& docs_map) {
  return {
      docs_map.Get("USERVER_RPS_CCONTROL").As<Policy>(),
      docs_map.Get("USERVER_RPS_CCONTROL_ENABLED").As<bool>(),
      docs_map.Get("USERVER_RPS_CCONTROL_ACTIVATED_FACTOR_METRIC").As<int>()};
}

}  // namespace impl

}  // namespace congestion_control

USERVER_NAMESPACE_END
