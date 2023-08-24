#include <userver/congestion_control/controllers/v2.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

namespace {
constexpr std::chrono::seconds kStepPeriod{1};
}  // namespace

void DumpMetric(utils::statistics::Writer& writer, const Stats& stats) {
  writer["is-enabled"] = stats.is_enabled ? 1 : 0;
  writer["is-fake-mode"] = stats.is_fake_mode ? 1 : 0;
  if (stats.current_limit) writer["current-limit"] = stats.current_limit;
  writer["enabled-seconds"] = stats.enabled_epochs;
}

Controller::Controller(const std::string& name, Sensor& sensor,
                       Limiter& limiter, Stats& stats, const Config& config)
    : name_(name),
      sensor_(sensor),
      limiter_(limiter),
      stats_(stats),
      config_(config) {}

void Controller::Start() {
  if (config_.enabled) {
    LOG_INFO() << fmt::format("Congestion controller {} has started", name_)
               << LogFakeMode();
    periodic_.Start("congestion_control", {kStepPeriod}, [this] { Step(); });
  } else {
    LOG_INFO() << fmt::format(
        "Congestion controller {} is disabled via static config, not starting",
        name_);
  }
}

std::string_view Controller::LogFakeMode() const {
  if (config_.fake_mode)
    return " (fake mode)";
  else if (!enabled_)
    return " (disabled via config or experiment)";
  else
    return "";
}

void Controller::Step() {
  auto current = sensor_.GetCurrent();
  auto limit = Update(current);
  if (!config_.fake_mode) {
    if (enabled_) {
      limiter_.SetLimit(limit);
    } else {
      limiter_.SetLimit({});
    }
  }

  if (limit.load_limit) {
    LOG_ERROR()
        << fmt::format(
               "Congestion Control {} is active, sensor ({}), limiter ({})",
               name_, current.ToLogString(), limit.ToLogString())
        << LogFakeMode();
  } else {
    LOG_TRACE() << fmt::format(
                       "Congestion Control {} is not active, sensor ({})",
                       name_, current.ToLogString())
                << LogFakeMode();
  }

  if (limit.load_limit.has_value()) stats_.enabled_epochs++;
  stats_.current_limit = limit.load_limit.value_or(0);
  stats_.is_enabled = limit.load_limit.has_value();
  stats_.is_fake_mode = config_.fake_mode || !enabled_;
}

const std::string& Controller::GetName() const { return name_; }

void Controller::SetEnabled(bool enabled) { enabled_ = enabled; }

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
