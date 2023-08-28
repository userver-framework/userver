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

namespace storages::redis {
class Client;
}  // namespace storages::redis

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
  HostVector resolved_host{};

  ConnectionInfo() = default;
  ConnectionInfo(std::string host, int port, Password password,
                 bool read_only = false,
                 ConnectionSecurity security = ConnectionSecurity::kNone,
                 HostVector resolved_host = {})
      : host{std::move(host)},
        port{port},
        password{std::move(password)},
        read_only{read_only},
        connection_security(security),
        resolved_host(std::move(resolved_host)) {}
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
  bool timings_enabled{true};
  bool command_timings_enabled{false};
  bool request_sizes_enabled{false};
  bool reply_sizes_enabled{false};

  constexpr bool operator==(const MetricsSettings& rhs) const {
    return timings_enabled == rhs.timings_enabled &&
           command_timings_enabled == rhs.command_timings_enabled &&
           request_sizes_enabled == rhs.request_sizes_enabled &&
           reply_sizes_enabled == rhs.reply_sizes_enabled;
  }

  constexpr bool operator!=(const MetricsSettings& rhs) const {
    return !(*this == rhs);
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

}  // namespace redis

USERVER_NAMESPACE_END
