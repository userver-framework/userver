#include <storages/mongo/tcp_connect_precheck.hpp>

#include <atomic>
#include <chrono>
#include <string>

#include <userver/rcu/rcu_map.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/token_bucket.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {
namespace {

constexpr size_t kRecentErrorThreshold = 2;
constexpr std::chrono::seconds kRecentErrorPeriod{15};

constexpr size_t kRecoveryAttemptsLimit = 1;
constexpr std::chrono::seconds kRecoveryPeriod{20};

struct InstanceState {
  std::atomic<bool> is_failed{false};

  // only used when not failed
  utils::TokenBucket failures{
      kRecentErrorThreshold,
      {1, utils::TokenBucket::Duration{kRecentErrorPeriod} /
              kRecentErrorThreshold}};

  // only used when failed
  utils::TokenBucket recovery_attempts{
      kRecoveryAttemptsLimit,
      {1,
       utils::TokenBucket::Duration{kRecoveryPeriod} / kRecoveryAttemptsLimit}};
};

using InstanceStatesMap = rcu::RcuMap<std::string, InstanceState>;

auto& GetInstanceStatesByHostAndPort() {
  static InstanceStatesMap instance_states_by_host_and_port;
  return instance_states_by_host_and_port;
}

}  // namespace

bool IsTcpConnectAllowed(const char* host_and_port) {
  auto instance_state = GetInstanceStatesByHostAndPort().Get(host_and_port);

  if (!instance_state || !instance_state->is_failed) return true;

  // we're in recovery mode
  if (instance_state->recovery_attempts.Obtain()) return true;

  return false;
}

void ReportTcpConnectSuccess(const char* host_and_port) {
  GetInstanceStatesByHostAndPort().Emplace(host_and_port).value->is_failed =
      false;
}

void ReportTcpConnectError(const char* host_and_port) {
  auto instance_state =
      GetInstanceStatesByHostAndPort().Emplace(host_and_port).value;
  UASSERT(instance_state);

  if (!instance_state->failures.Obtain()) instance_state->is_failed = true;
}

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
