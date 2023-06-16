#pragma once

#include <optional>

#include <userver/congestion_control/controllers/linear_config.hpp>
#include <userver/congestion_control/controllers/v2.hpp>
#include <userver/congestion_control/limiter.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/utils/smoothed_value.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

class LinearController final : public Controller {
 public:
  using StaticConfig = Controller::Config;

  LinearController(
      const std::string& name, v2::Sensor& sensor, Limiter& limiter,
      Stats& stats, const StaticConfig& config,
      dynamic_config::Source config_source,
      std::function<v2::Config(const dynamic_config::Snapshot&)> config_getter);

  Limit Update(const Sensor::Data& current) override;

 private:
  StaticConfig config_;
  utils::SmoothedValue<int64_t> current_load_;
  utils::SmoothedValue<int64_t> long_timings_;
  utils::MinimalValue<int64_t> short_timings_;
  std::optional<std::size_t> current_limit_;
  std::size_t epochs_passed_{0};

  dynamic_config::Source config_source_;
  std::function<v2::Config(const dynamic_config::Snapshot&)> config_getter_;
};

LinearController::StaticConfig Parse(
    const yaml_config::YamlConfig& value,
    formats::parse::To<LinearController::StaticConfig>);

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
