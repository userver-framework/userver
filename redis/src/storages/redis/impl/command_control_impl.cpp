#include "command_control_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

CommandControlImpl::CommandControlImpl(const CommandControl& command_control) {
  if (command_control.timeout_single.has_value())
    timeout_single = *command_control.timeout_single;
  if (command_control.timeout_all.has_value())
    timeout_all = *command_control.timeout_all;
  if (command_control.max_retries.has_value())
    max_retries = *command_control.max_retries;
  if (command_control.strategy.has_value())
    strategy = *command_control.strategy;
  if (command_control.best_dc_count.has_value())
    best_dc_count = *command_control.best_dc_count;
  if (command_control.force_request_to_master.has_value())
    force_request_to_master = *command_control.force_request_to_master;
  if (command_control.max_ping_latency.has_value())
    max_ping_latency = *command_control.max_ping_latency;
  if (command_control.allow_reads_from_master.has_value())
    allow_reads_from_master = *command_control.allow_reads_from_master;
  if (command_control.account_in_statistics.has_value())
    account_in_statistics = *command_control.account_in_statistics;
  if (command_control.chunk_size.has_value())
    chunk_size = *command_control.chunk_size;
  if (command_control.force_server_id.has_value())
    force_server_id = *command_control.force_server_id;
}

}  // namespace redis

USERVER_NAMESPACE_END
