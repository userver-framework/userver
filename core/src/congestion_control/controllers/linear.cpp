#include <userver/congestion_control/controllers/linear.hpp>

#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

namespace {
constexpr std::size_t kCurrentLoadEpochs = 3;
constexpr std::size_t kShortTimingsEpochs = 3;
constexpr std::size_t kLongTimingsEpochs = 30;
}  // namespace

LinearController::LinearController(
    const std::string& name, v2::Sensor& sensor, Limiter& limiter, Stats& stats,
    const StaticConfig& config, dynamic_config::Source config_source,
    std::function<v2::Config(const dynamic_config::Snapshot&)> config_getter)
    : Controller(name, sensor, limiter, stats,
                 {config.fake_mode, config.enabled}),
      config_(config),
      current_load_(kCurrentLoadEpochs),
      long_timings_(kLongTimingsEpochs),
      short_timings_(kShortTimingsEpochs),
      config_source_(config_source),
      config_getter_(config_getter) {}

Limit LinearController::Update(const Sensor::Data& current) {
  auto dyn_config = config_source_.GetSnapshot();
  v2::Config config = config_getter_(dyn_config);

  auto rate = current.GetRate();

  short_timings_.Update(current.timings_avg_ms);

  current_load_.Update(current.current_load);
  auto current_load = current_load_.GetSmoothed();

  bool overloaded = 100 * rate > config.errors_threshold_percent;

  std::size_t divisor = std::max<std::size_t>(long_timings_.GetSmoothed(),
                                              config.min_timings.count());

  LOG_DEBUG() << "CC mongo:"
              << " sensor=(" << current.ToLogString() << ") divisor=" << divisor
              << " short_timings_.GetMinimal()=" << short_timings_.GetMinimal()
              << " long_timings_.GetSmoothed()=" << long_timings_.GetSmoothed();

  if (current.total < config.min_qps && !current_limit_) {
    // Too little QPS, timings avg data is VERY noisy, EPS is noisy
    return {current_limit_, current.current_load};
  }

  if (epochs_passed_ < kLongTimingsEpochs) {
    // First seconds of service life might be too noisy
    epochs_passed_++;
    long_timings_.Update(current.timings_avg_ms);
    return {std::nullopt, current.current_load};
  }

  if (static_cast<std::size_t>(short_timings_.GetMinimal()) >
      config.timings_burst_threshold * divisor) {
    // Do not update long_timings_, it is sticky to "good" timings
    overloaded = true;
  } else {
    long_timings_.Update(current.timings_avg_ms);
  }

  if (overloaded) {
    if (current_limit_) {
      *current_limit_ = *current_limit_ * 0.95;
    } else {
      LOG_ERROR() << GetName() << " Congestion Control is activated";
      current_limit_ = current_load;
    }
  } else {
    if (current_limit_) {
      if (current_limit_ > current_load + config.safe_delta_limit) {
        // TODO: several seconds in a row
        if (current_limit_.has_value()) {
          LOG_ERROR() << GetName() << " Congestion Control is deactivated";
        }

        current_limit_.reset();
      } else {
        ++*current_limit_;
      }
    }
  }

  if (current_limit_.has_value() && current_limit_ < config.min_limit) {
    current_limit_ = config.min_limit;
  }

  return {current_limit_, current.current_load};
}

LinearController::StaticConfig Parse(
    const yaml_config::YamlConfig& value,
    formats::parse::To<LinearController::StaticConfig>) {
  LinearController::StaticConfig config;
  config.fake_mode = value["fake-mode"].As<bool>(false);
  config.enabled = value["enabled"].As<bool>(true);
  return config;
}

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
