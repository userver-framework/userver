#pragma once

/// @file userver/alert/storage.hpp
/// @brief @copybrief alert::Storage

#include <chrono>
#include <string>
#include <vector>

#include <userver/concurrent/variable.hpp>

USERVER_NAMESPACE_BEGIN

namespace alerts {

struct Alert final {
  std::string id;
  std::string description;
  std::chrono::steady_clock::time_point stop_timepoint;
};

inline constexpr auto kDefaultDuration = std::chrono::seconds{120};
inline constexpr auto kInfinity = std::chrono::hours{24 * 999};

/// @ingroup userver_clients
///
/// @brief Storage for active fired alerts.
class Storage final {
 public:
  Storage() = default;
  Storage(const Storage&) = delete;
  Storage(Storage&&) = delete;

  /// Fire an alert. It will be stopped either after `StopAlertNow` for the
  /// `alert_id` is called or after `duration` seconds.
  void FireAlert(std::string_view alert_id, std::string_view description,
                 std::chrono::seconds duration = kDefaultDuration) noexcept;

  /// Stop an alert before its duration has ended.
  void StopAlertNow(std::string_view alert_id) noexcept;

  /// Collect fired and active alerts.
  std::vector<Alert> CollectActiveAlerts();

 private:
  static void DoStopAlertNow(std::string_view alert_id,
                             std::vector<Alert>& alerts);

  concurrent::Variable<std::vector<Alert>> alerts_;
};

}  // namespace alerts

USERVER_NAMESPACE_END
