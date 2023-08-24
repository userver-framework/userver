#include <userver/storages/redis/impl/wait_connected_mode.hpp>

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace redis {

std::string ToString(WaitConnectedMode mode) {
  switch (mode) {
    case WaitConnectedMode::kNoWait:
      return "no_wait";
    case WaitConnectedMode::kMaster:
      return "master";
    case WaitConnectedMode::kSlave:
      return "slave";
    case WaitConnectedMode::kMasterOrSlave:
      return "master_or_slave";
    case WaitConnectedMode::kMasterAndSlave:
      return "master_and_slave";
  }
  throw std::runtime_error("unknown mode: " +
                           std::to_string(static_cast<int>(mode)));
}

WaitConnectedMode Parse(const std::string& str,
                        formats::parse::To<WaitConnectedMode>) {
  if (str == "no_wait") return WaitConnectedMode::kNoWait;
  if (str == "master") return WaitConnectedMode::kMaster;
  if (str == "slave") return WaitConnectedMode::kSlave;
  if (str == "master_or_slave") return WaitConnectedMode::kMasterOrSlave;
  if (str == "master_and_slave") return WaitConnectedMode::kMasterAndSlave;
  throw std::runtime_error("can't parse WaitConnectedMode from '" + str + '\'');
}

RedisWaitConnected RedisWaitConnected::MergeWith(
    const testsuite::RedisControl& t) const {
  RedisWaitConnected result(*this);
  result.timeout = std::max(t.min_timeout_connect, result.timeout);
  return result;
}
}  // namespace redis

USERVER_NAMESPACE_END
