#pragma once

/// @file userver/ydb/dist_lock/settings.hpp
/// @brief @copybrief ydb::DistLockSettings

#include <chrono>

#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

/// A set of tweak-able settings for ydb::DistLockWorker.
struct DistLockSettings {
  /// @brief For how long we will try to restore session after a network failure
  /// before dropping it. Corresponds to `TSessionSettings::Timeout`.
  ///
  /// If the network connection to the coordination node is temporarily lost,
  /// some normal reconnect attempts will be performed
  /// within `session_timeout`.
  /// After the session finally gives up, YDB Coordinator drops the active lock
  /// held by the current host, if any.
  std::chrono::milliseconds session_timeout{5000};

  /// @brief Backoff before attempting to reconnect session after it returns
  /// "permanent failure".
  ///
  /// If the network connection to the coordination node is temporarily lost,
  /// some normal reconnect attempts will be performed
  /// within `session_timeout`.
  /// After the session finally gives up, ydb::DistLockWorker will attempt
  /// to restart the coordination session after this time.
  std::chrono::milliseconds restart_session_delay{1000};

  /// Backoff before repeating a failed Acquire call.
  std::chrono::milliseconds acquire_interval{100};

  /// Backoff before calling DoWork again after it returns or throws.
  std::chrono::milliseconds restart_delay{100};

  /// @brief Time, within which a cancelled `DoWork` is expected to finish.
  ///
  /// Ignoring cancellations for this long can lead to a "brain split" where
  /// `DoWork` is running on multiple hosts. Practically speaking, this time
  /// can be computed as:
  ///
  /// `TNodeSettings::SessionGracePeriod - TSessionSettings::Timeout`
  ///
  /// and should be set to that or a smaller value.
  std::chrono::milliseconds cancel_task_time_limit{5000};
};

}  // namespace ydb

namespace formats::parse {

// ydb::Parse is a variable template, thus have to define formats Parse here.
ydb::DistLockSettings Parse(const yaml_config::YamlConfig& config,
                            To<ydb::DistLockSettings>);

}  // namespace formats::parse

USERVER_NAMESPACE_END
