#include <userver/congestion_control/controllers/v2.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

namespace {
constexpr std::chrono::seconds kStepPeriod{1};
}  // namespace

Controller::Controller(Sensor& sensor, Limiter& limiter)
    : sensor_(sensor), limiter_(limiter) {}

void Controller::Start() {
  LOG_INFO() << "Congestion controller for pool XXX has started";  // TODO: dsn?
  periodic_.Start("congestion_control", {kStepPeriod}, [this] { Step(); });
}

void Controller::Step() {
  auto current = sensor_.GetCurrent();
  auto limit = Update(current);
  limiter_.SetLimit(limit);
}

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
