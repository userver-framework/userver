#include <userver/alerts/storage.hpp>

#include <algorithm>

#include <userver/logging/log.hpp>
#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace alerts {

void Storage::FireAlert(std::string_view alert_id, std::string_view description,
                        std::chrono::seconds duration) noexcept {
  try {
    // Allocate outside of critical section
    std::string id(alert_id);
    std::string message(description);

    LOG_LIMITED_ERROR() << description << logging::LogExtra{{"alert_id", id}};

    auto alerts = alerts_.Lock();
    DoStopAlertNow(alert_id, *alerts);
    alerts->push_back({std::move(id), std::move(message),
                       utils::datetime::SteadyNow() + duration});
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to fire alert: " << e;
  }
}

void Storage::StopAlertNow(std::string_view alert_id) noexcept {
  try {
    auto alerts = alerts_.Lock();

    DoStopAlertNow(alert_id, *alerts);
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to stop alert: " << e;
  }
}

void Storage::DoStopAlertNow(std::string_view alert_id,
                             std::vector<Alert>& alerts) {
  alerts.erase(
      std::remove_if(alerts.begin(), alerts.end(),
                     [alert_id](const Alert& a) { return a.id == alert_id; }),
      alerts.end());
}

std::vector<Alert> Storage::CollectActiveAlerts() {
  auto now = utils::datetime::SteadyNow();
  auto alerts = alerts_.Lock();

  alerts->erase(
      std::remove_if(alerts->begin(), alerts->end(),
                     [now](const Alert& a) { return a.stop_timepoint < now; }),
      alerts->end());

  return *alerts;
}

}  // namespace alerts

USERVER_NAMESPACE_END
