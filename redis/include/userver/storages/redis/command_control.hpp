#pragma once

/// @file userver/storages/redis/command_control.hpp
/// @brief @copybrief redis::CommandControl

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#include <userver/storages/redis/impl/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {
struct RedisControl;
}

namespace storages::redis {
class Client;
}  // namespace storages::redis

namespace redis {

/// Opaque Id of Redis server instance / any server instance.
class ServerId {
 public:
  /// Default: any server
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
  static std::atomic<std::int64_t> next_id_;
  static ServerId invalid_;

  std::int64_t id_{-1};
};

struct ServerIdHasher {
  std::size_t operator()(ServerId server_id) const noexcept {
    return std::hash<std::size_t>{}(server_id.GetId());
  }
};

struct RetryNilFromMaster {};

/// Can be used as an additional parameter in some commands to force retries to
/// master if slave returned a nil reply.
inline constexpr RetryNilFromMaster kRetryNilFromMaster{};

/// Redis command execution options
struct CommandControl {
  enum class Strategy {
    /// Same as kEveryDc
    kDefault,

    /// Send ~1/N requests to an instance with ping N ms
    kEveryDc,

    /// Send requests to Redis instances located in local DC (by Conductor info)
    kLocalDcConductor,

    /// Send requests to 'best_dc_count' Redis instances with the min ping
    kNearestServerPing,
  };

  /// Timeout for a single attempt to execute command
  std::chrono::milliseconds timeout_single = std::chrono::milliseconds{500};

  /// Command execution timeout, including retries
  std::chrono::milliseconds timeout_all = std::chrono::milliseconds{2000};

  /// The maximum number of retries while executing command
  size_t max_retries = 4;

  /// Server instance selection strategy
  Strategy strategy = Strategy::kDefault;

  /// How many nearest DCs to use, 0 for no limit
  size_t best_dc_count = 0;

  /// Server latency limit
  std::chrono::milliseconds max_ping_latency = std::chrono::milliseconds(0);

  /// Force execution on master node
  bool force_request_to_master = false;

  /// Allow execution of readonly commands on master node along with replica
  /// nodes to facilitate load distribution.
  bool allow_reads_from_master = false;

  /// Controls if the command execution accounted in statistics
  bool account_in_statistics = true;

  /// If set, force execution on specific shard
  std::optional<std::size_t> force_shard_idx;

  /// Split execution of multi-key commands (i.e., MGET) to multiple requests
  std::size_t chunk_size = 0;

  /// If set, the user wants a specific Redis instance to handle the command.
  /// Sentinel may not redirect the command to other instances. strategy is
  /// ignored.
  ServerId force_server_id;

  /// If set, command retries are directed to the master instance
  bool force_retries_to_master_on_nil_reply = false;

  CommandControl() = default;
  CommandControl(std::chrono::milliseconds timeout_single,
                 std::chrono::milliseconds timeout_all, std::size_t max_retries,
                 Strategy strategy = Strategy::kDefault, int best_dc_count = 0,
                 std::chrono::milliseconds max_ping_latency =
                     std::chrono::milliseconds(0));

  CommandControl MergeWith(const CommandControl& b) const;
  CommandControl MergeWith(const testsuite::RedisControl&) const;
  CommandControl MergeWith(RetryNilFromMaster) const;

  std::string ToString() const;

  friend class Sentinel;
  friend class storages::redis::Client;
};

/// Returns CommandControl::Strategy from string
CommandControl::Strategy StrategyFromString(std::string_view s);

}  // namespace redis

USERVER_NAMESPACE_END
