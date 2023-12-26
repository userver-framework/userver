#pragma once

#include <userver/storages/redis/command_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

struct CommandControlImpl {
 public:
  using Strategy = CommandControl::Strategy;

  explicit CommandControlImpl(const CommandControl& command_control);

  /// Timeout for a single attempt to execute command
  std::chrono::milliseconds timeout_single{kDefaultTimeoutSingle};

  /// Command execution timeout, including retries
  std::chrono::milliseconds timeout_all{kDefaultTimeoutAll};

  /// The maximum number of retries while executing command
  std::size_t max_retries{kDefaultMaxRetries};

  /// Server instance selection strategy
  Strategy strategy{Strategy::kDefault};

  /// How many nearest DCs to use, 0 for no limit
  std::size_t best_dc_count{0};

  /// Server latency limit
  std::chrono::milliseconds max_ping_latency{0};

  /// Allow execution of readonly commands on master node along with replica
  /// nodes to facilitate load distribution
  bool allow_reads_from_master{false};

  /// Force execution on master node
  bool force_request_to_master{false};

  /// Controls if the command execution accounted in statistics
  bool account_in_statistics{true};

  /// Split execution of multi-key commands (i.e., MGET) to multiple requests
  std::size_t chunk_size{0};

  /// If set, the user wants a specific Redis instance to handle the command.
  /// Sentinel may not redirect the command to other instances. strategy is
  /// ignored.
  ServerId force_server_id;
};

}  // namespace redis

USERVER_NAMESPACE_END
