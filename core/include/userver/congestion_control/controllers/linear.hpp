#pragma once

#include <optional>

#include <userver/congestion_control/controllers/v2.hpp>
#include <userver/congestion_control/limiter.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/utils/smoothed_value.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

class LinearController final : public Controller {
 public:
  struct StaticConfig {
    int64_t safe_limit{100};
    double threshold_percent{5.0};

    bool fake_mode{false};
    bool enabled{true};
  };

  LinearController(const std::string& name, v2::Sensor& sensor,
                   Limiter& limiter, Stats& stats, const StaticConfig& config);

  Limit Update(const Sensor::Data& current) override;

 private:
  StaticConfig config_;
  utils::SmoothedValue<int64_t> current_load_;
  utils::SmoothedValue<int64_t> long_timings_;
  utils::SmoothedValue<int64_t> short_timings_;
  std::optional<size_t> current_limit_;
  size_t epochs_passed_{0};
};

LinearController::StaticConfig Parse(
    const yaml_config::YamlConfig& value,
    formats::parse::To<LinearController::StaticConfig>);

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
