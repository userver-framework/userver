#pragma once

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <userver/logging/fwd.hpp>
#include <userver/utils/strong_typedef.hpp>

#include <userver/storages/redis/command_control.hpp>
#include <userver/storages/redis/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

using Password = utils::NonLoggable<class PasswordTag, std::string>;

enum class ConnectionSecurity { kNone, kTLS };

struct ConnectionInfo {
  std::string host = "localhost";
  int port = 26379;
  Password password;
  bool read_only = false;
  ConnectionSecurity connection_security = ConnectionSecurity::kNone;
  using HostVector = std::vector<std::string>;

  ConnectionInfo() = default;
  ConnectionInfo(std::string host, int port, Password password,
                 bool read_only = false,
                 ConnectionSecurity security = ConnectionSecurity::kNone)
      : host{std::move(host)},
        port{port},
        password{std::move(password)},
        read_only{read_only},
        connection_security(security) {}
};

struct Stat {
  double tps = 0.0;
  double queue = 0.0;
  double inprogress = 0.0;
  double timeouts = 0.0;
};

class CmdArgs {
 public:
  using CmdArgsArray = std::vector<std::string>;
  using CmdArgsChain = std::vector<CmdArgsArray>;

  CmdArgs() = default;

  template <typename... Args>
  CmdArgs(Args&&... _args) {
    Then(std::forward<Args>(_args)...);
  }

  CmdArgs(const CmdArgs& o) = delete;
  CmdArgs(CmdArgs&& o) = default;

  CmdArgs& operator=(const CmdArgs& o) = delete;
  CmdArgs& operator=(CmdArgs&& o) = default;

  template <typename... Args>
  CmdArgs& Then(Args&&... _args);

  CmdArgs Clone() const {
    CmdArgs r;
    r.args = args;
    return r;
  }

  CmdArgsChain args;
};

logging::LogHelper& operator<<(logging::LogHelper& os, const CmdArgs& v);

using ScanCursor = int64_t;

template <typename Arg>
typename std::enable_if<std::is_arithmetic<Arg>::value, void>::type PutArg(
    CmdArgs::CmdArgsArray& args_, const Arg& arg) {
  args_.emplace_back(std::to_string(arg));
}

void PutArg(CmdArgs::CmdArgsArray& args_, const char* arg);

void PutArg(CmdArgs::CmdArgsArray& args_, const std::string& arg);

void PutArg(CmdArgs::CmdArgsArray& args_, std::string&& arg);

void PutArg(CmdArgs::CmdArgsArray& args_, const std::vector<std::string>& arg);

void PutArg(CmdArgs::CmdArgsArray& args_,
            const std::vector<std::pair<std::string, std::string>>& arg);

void PutArg(CmdArgs::CmdArgsArray& args_,
            const std::vector<std::pair<double, std::string>>& arg);

template <typename... Args>
CmdArgs& CmdArgs::Then(Args&&... _args) {
  args.emplace_back();
  auto& new_args = args.back();
  new_args.reserve(sizeof...(Args));
  (PutArg(new_args, std::forward<Args>(_args)), ...);
  return *this;
}

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

enum class ConnectionMode {
  kCommands,
  kSubscriber,
};

struct MetricsSettings {
  enum class Level { kCluster, kShard, kInstance };

  struct DynamicSettings {
    bool timings_enabled{true};
    bool command_timings_enabled{false};
    bool request_sizes_enabled{false};
    bool reply_sizes_enabled{false};

    constexpr bool operator==(const DynamicSettings& rhs) const {
      return timings_enabled == rhs.timings_enabled &&
             command_timings_enabled == rhs.command_timings_enabled &&
             request_sizes_enabled == rhs.request_sizes_enabled &&
             reply_sizes_enabled == rhs.reply_sizes_enabled;
    }

    constexpr bool operator!=(const DynamicSettings& rhs) const {
      return !(*this == rhs);
    }
  };

  struct StaticSettings {
    Level level{Level::kInstance};

    constexpr bool operator==(const StaticSettings& rhs) const {
      return level == rhs.level;
    }

    constexpr bool operator!=(const StaticSettings& rhs) const {
      return !(*this == rhs);
    }
  };

  StaticSettings static_settings;
  DynamicSettings dynamic_settings;

  MetricsSettings(const DynamicSettings& dynamic_settings,
                  const StaticSettings& static_settings)
      : static_settings(static_settings), dynamic_settings(dynamic_settings) {}
  MetricsSettings() = default;
  MetricsSettings(const MetricsSettings&) = default;
  MetricsSettings(MetricsSettings&&) = default;
  MetricsSettings& operator=(const MetricsSettings&) = default;
  MetricsSettings& operator=(MetricsSettings&&) = default;

  constexpr bool operator==(const MetricsSettings& rhs) const {
    return static_settings == rhs.static_settings &&
           dynamic_settings == rhs.dynamic_settings;
  }

  constexpr bool operator!=(const MetricsSettings& rhs) const {
    return !(*this == rhs);
  }

  Level GetMetricsLevel() const { return static_settings.level; }
  bool IsTimingsEnabled() const { return dynamic_settings.timings_enabled; }
  bool IsCommandTimingsEnabled() const {
    return dynamic_settings.command_timings_enabled;
  }
  bool IsRequestSizesEnabled() const {
    return dynamic_settings.request_sizes_enabled;
  }
  bool IsReplySizesEnabled() const {
    return dynamic_settings.reply_sizes_enabled;
  }
};

struct PubsubMetricsSettings {
  bool per_shard_stats_enabled{true};

  constexpr bool operator==(const PubsubMetricsSettings& rhs) const {
    return per_shard_stats_enabled == rhs.per_shard_stats_enabled;
  }

  constexpr bool operator!=(const PubsubMetricsSettings& rhs) const {
    return !(*this == rhs);
  }
};

struct ReplicationMonitoringSettings {
  bool enable_monitoring{false};
  bool restrict_requests{false};
};

struct PublishSettings {
  size_t shard{0};
  bool master{true};
  CommandControl::Strategy strategy{CommandControl::Strategy::kDefault};
};

}  // namespace redis

USERVER_NAMESPACE_END
