#include <userver/congestion_control/controllers/linear.hpp>

#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

namespace {
constexpr double kTimeoutThreshold = 5.0;  // 5%
constexpr size_t kSafeDeltaLimit = 10;
constexpr size_t kCurrentLoadEpochs = 3;
constexpr size_t kLongTimingsEpochs = 30;
constexpr size_t kTimingsBurstThreshold = 5;
constexpr size_t kMinTimingsMs = 20;
constexpr size_t kMinLimit = 10;
constexpr size_t kMinQps = 10;
}  // namespace

LinearController::LinearController(const std::string& name, v2::Sensor& sensor,
                                   Limiter& limiter, Stats& stats,
                                   const StaticConfig& config)
    : Controller(name, sensor, limiter, stats,
                 {config.fake_mode, config.enabled}),
      config_(config),
      current_load_(kCurrentLoadEpochs),
      long_timings_(kLongTimingsEpochs) {
  LOG_DEBUG() << "Linear Congestion-Control is created with the following "
                 "config: safe_limit="
              << config_.safe_limit
              << ", threshold_percent=" << config_.threshold_percent;
}

Limit LinearController::Update(const Sensor::Data& current) {
  auto rate = current.GetRate();
  auto timings_avg_ms = current.timings_avg_ms;

  current_load_.Update(current.current_load);
  auto current_load = current_load_.GetSmoothed();

  bool overloaded = 100 * rate > config_.threshold_percent;

  if (epochs_passed_ < kLongTimingsEpochs) {
    if (current.total > kMinQps) {
      epochs_passed_++;
      long_timings_.Update(timings_avg_ms);
    } else {
      // Too little QPS, timings avg data is VERY noisy, skip it
    }
  } else {
    size_t divisor = long_timings_.GetSmoothed();
    if (divisor < kMinTimingsMs) divisor = kMinTimingsMs;
    if (current.timings_avg_ms > kTimingsBurstThreshold * divisor) {
      overloaded = true;
    } else {
      long_timings_.Update(current.timings_avg_ms);
    }
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
      if (current_limit_ > current_load + config_.safe_limit) {
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

  if (current_limit_.has_value() && current_limit_ < kMinLimit) {
    current_limit_ = kMinLimit;
  }

  return {current_limit_, current.current_load};
}

LinearController::StaticConfig Parse(
    const yaml_config::YamlConfig& value,
    formats::parse::To<LinearController::StaticConfig>) {
  LinearController::StaticConfig config;
  config.safe_limit = value["safe-limit"].As<int64_t>(kSafeDeltaLimit);
  config.threshold_percent =
      value["threshold-percent"].As<double>(kTimeoutThreshold);
  config.fake_mode = value["fake-mode"].As<bool>(false);
  config.enabled = value["enabled"].As<bool>(true);
  return config;
}

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
