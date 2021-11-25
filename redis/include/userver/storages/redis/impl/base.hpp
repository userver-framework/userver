#pragma once

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <userver/testsuite/redis_control.hpp>
#include <userver/utils/strong_typedef.hpp>

#include <userver/storages/redis/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {
class Client;
}  // namespace storages::redis

namespace redis {

#ifndef REDIS_ERR_TIMEOUT
const int REDIS_ERR_TIMEOUT = 6;
#endif

const int REDIS_ERR_NOT_READY = 7;
const int REDIS_ERR_MAX = REDIS_ERR_NOT_READY + 1;

using Password = utils::NonLoggable<class PasswordTag, std::string>;

struct ConnectionInfo {
  std::string host = "localhost";
  int port = 26379;
  Password password;

  ConnectionInfo() = default;
  ConnectionInfo(std::string host, int port, Password password)
      : host(std::move(host)), port(port), password(std::move(password)) {}
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

  std::string ToString() const;

  CmdArgs Clone() const {
    CmdArgs r;
    r.args = args;
    return r;
  }

  CmdArgsChain args;
};

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

  using expand_type = int[];
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif
  expand_type{(PutArg(new_args, std::forward<Args>(_args)), 0)...};
#ifdef __clang__
#pragma clang diagnostic pop
#endif
  return *this;
}

/* Opaque Id of Redis server instance / any server instance */
class ServerId {
 public:
  /* Default: any server */
  ServerId() = default;

  bool IsAny() const { return id_ == -1; }

  bool operator==(const ServerId& other) const { return other.id_ == id_; }
  bool operator!=(const ServerId& other) const { return !(other == *this); }

  bool operator<(const ServerId& other) const { return id_ < other.id_; }

  static ServerId Generate() {
    ServerId sid;
    sid.id_ = next_id_++;
    return sid;
  }

  static ServerId Invalid() { return invalid_; }

  int64_t GetId() const { return id_; }

  void SetDescription(std::string description) const;
  void RemoveDescription() const;
  std::string GetDescription() const;

 private:
  static std::atomic<int64_t> next_id_;
  static ServerId invalid_;

  int64_t id_{-1};
};

struct ServerIdHasher {
  size_t operator()(ServerId server_id) const {
    return std::hash<size_t>{}(server_id.GetId());
  }
};

struct RetryNilFromMaster {};
// Can be used as an additional parameter in some commands to force retries to
// master if slave returned a nil reply.
static const RetryNilFromMaster kRetryNilFromMaster{};

struct CommandControl {
  enum class Strategy {
    /* same as kEveryDc */
    kDefault,
    /* Send ~1/N requests to an instance with ping N ms */
    kEveryDc,
    /* Send requests to Redis instances located in local DC (by Conductor
       info) */
    kLocalDcConductor,
    /* Send requests to 'best_dc_count' Redis instances with the min ping */
    kNearestServerPing,
  };

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::chrono::milliseconds timeout_single = std::chrono::milliseconds(0);
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::chrono::milliseconds timeout_all = std::chrono::milliseconds(0);
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  size_t max_retries = 0;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  Strategy strategy = Strategy::kDefault;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  size_t best_dc_count = 0; /* How many nearest DCs to use, 0 for no limit */
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::chrono::milliseconds max_ping_latency = std::chrono::milliseconds(0);
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  bool force_request_to_master = false;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  bool account_in_statistics = true;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::optional<size_t> force_shard_idx;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  size_t chunk_size = 0;

  /* If set, the user wants a specific Redis instance to handle the command.
   * Sentinel may not redirect the command to other instances.
   * strategy is ignored.
   */
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  ServerId force_server_id;

  CommandControl() = default;
  CommandControl(std::chrono::milliseconds timeout_single,
                 std::chrono::milliseconds timeout_all, size_t max_retries,
                 Strategy strategy = Strategy::kDefault, int best_dc_count = 0,
                 std::chrono::milliseconds max_ping_latency =
                     std::chrono::milliseconds(0));

  CommandControl MergeWith(const CommandControl& b) const;
  CommandControl MergeWith(const testsuite::RedisControl&) const;

  std::string ToString() const;

  bool GetForceRetriesToMasterOnNilReply() const;

  friend class Sentinel;
  friend class storages::redis::Client;

 private:
  CommandControl MergeWith(RetryNilFromMaster) const;
  std::optional<bool> force_retries_to_master_on_nil_reply;
};

CommandControl::Strategy StrategyFromString(const std::string& s);

const CommandControl command_control_init = {
    /*.timeout_single = */ std::chrono::milliseconds(500),
    /*.timeout_all = */ std::chrono::milliseconds(2000),
    /*.max_retries = */ 4};

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

}  // namespace redis

USERVER_NAMESPACE_END
