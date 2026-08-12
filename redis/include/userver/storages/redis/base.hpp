#pragma once

/// @file userver/storages/redis/base.hpp
/// @brief Redis connection parameters, metrics, buffering and publish settings

#include <chrono>
#include <string>
#include <vector>

#include <userver/logging/fwd.hpp>
#include <userver/utils/strong_typedef.hpp>

#include <userver/storages/redis/command_control.hpp>
#include <userver/storages/redis/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

using Password = utils::NonLoggable<class PasswordTag, std::string>;

/// @brief Redis node authentication credentials (ACL-aware).
///
/// When `username` is empty, the legacy `AUTH <password>` command is used.
/// When `username` is non-empty, the ACL `AUTH <username> <password>` command
/// is used (requires Redis 6+).
struct Credentials {
    std::string username;
    Password password;
};

enum class ConnectionSecurity { kNone, kTLS };

struct ConnectionInfo {
    std::string host = "localhost";
    int port = 26379;
    Credentials credentials;
    bool read_only = false;
    ConnectionSecurity connection_security = ConnectionSecurity::kNone;
    using HostVector = std::vector<std::string>;
    std::size_t database_index = 0;

    ConnectionInfo() = default;
    ConnectionInfo(
        std::string host,
        int port,
        Credentials credentials,
        bool read_only = false,
        ConnectionSecurity security = ConnectionSecurity::kNone,
        std::size_t db_index = 0
    )
        : host{std::move(host)},
          port{port},
          credentials{std::move(credentials)},
          read_only{read_only},
          connection_security(security),
          database_index(db_index)
    {}
};

struct Stat {
    double tps = 0.0;
    double queue = 0.0;
    double inprogress = 0.0;
    double timeouts = 0.0;
};

using ScanCursor = int64_t;

struct CommandsBufferingSettings {
    bool buffering_enabled{false};
    size_t commands_buffering_threshold{0};
    std::chrono::microseconds watch_command_timer_interval{0};

    constexpr bool operator==(const CommandsBufferingSettings& o) const {
        return buffering_enabled == o.buffering_enabled &&
               commands_buffering_threshold == o.commands_buffering_threshold &&
               watch_command_timer_interval == o.watch_command_timer_interval;
    }
};

struct MetricsSettings {
    struct DynamicSettings {
        bool timings_enabled{true};
        bool command_timings_enabled{false};
        bool request_sizes_enabled{false};
        bool reply_sizes_enabled{false};

        constexpr bool operator==(const DynamicSettings& rhs) const {
            return timings_enabled == rhs.timings_enabled && command_timings_enabled == rhs.command_timings_enabled &&
                   request_sizes_enabled == rhs.request_sizes_enabled && reply_sizes_enabled == rhs.reply_sizes_enabled;
        }
    };

    DynamicSettings dynamic_settings;

    MetricsSettings(const DynamicSettings& dynamic_settings)
        : dynamic_settings(dynamic_settings)
    {}
    MetricsSettings() = default;
    MetricsSettings(const MetricsSettings&) = default;
    MetricsSettings(MetricsSettings&&) = default;
    MetricsSettings& operator=(const MetricsSettings&) = default;
    MetricsSettings& operator=(MetricsSettings&&) = default;

    constexpr bool operator==(const MetricsSettings& rhs) const { return dynamic_settings == rhs.dynamic_settings; }

    bool IsTimingsEnabled() const { return dynamic_settings.timings_enabled; }
    bool IsCommandTimingsEnabled() const { return dynamic_settings.command_timings_enabled; }
    bool IsRequestSizesEnabled() const { return dynamic_settings.request_sizes_enabled; }
    bool IsReplySizesEnabled() const { return dynamic_settings.reply_sizes_enabled; }
};

struct PubsubMetricsSettings {
    bool per_shard_stats_enabled{true};

    constexpr bool operator==(const PubsubMetricsSettings& rhs) const {
        return per_shard_stats_enabled == rhs.per_shard_stats_enabled;
    }
};

struct ReplicationMonitoringSettings {
    bool enable_monitoring{false};
    bool restrict_requests{false};
};

struct PublishSettings {
    size_t shard{0};
    bool master{true};
    storages::redis::CommandControl::Strategy strategy{storages::redis::CommandControl::Strategy::kDefault};
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
