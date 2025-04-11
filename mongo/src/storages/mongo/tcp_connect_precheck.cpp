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

constexpr size_t kRecentErrorThreshold = 5;

constexpr size_t kRecoveryAttemptsLimit = 1;
constexpr std::chrono::seconds kRecoveryPeriod{3};

struct InstanceState {
    std::atomic<uint64_t> failed_in_row{0};

    // only used when failed
    utils::TokenBucket recovery_attempts{
        kRecoveryAttemptsLimit,
        {1, utils::TokenBucket::Duration{kRecoveryPeriod} / kRecoveryAttemptsLimit}};
};

using InstanceStatesMap = rcu::RcuMap<std::string, InstanceState>;

auto& GetInstanceStatesByHostAndPort() {
    static InstanceStatesMap instance_states_by_host_and_port;
    return instance_states_by_host_and_port;
}

}  // namespace

HostConnectionState CheckTcpConnectionState(const char* host_and_port) {
    auto instance_state = GetInstanceStatesByHostAndPort().Get(host_and_port);

    if (!instance_state || instance_state->failed_in_row < kRecentErrorThreshold) return HostConnectionState::kAlive;

    // we're in recovery mode
    if (instance_state->recovery_attempts.Obtain()) return HostConnectionState::kChecking;

    return HostConnectionState::kDead;
}

void ReportTcpConnectSuccess(const char* host_and_port) {
    GetInstanceStatesByHostAndPort().Emplace(host_and_port).value->failed_in_row = 0;
}

void ReportTcpConnectError(const char* host_and_port) {
    auto instance_state = GetInstanceStatesByHostAndPort().Emplace(host_and_port).value;
    UASSERT(instance_state);

    instance_state->failed_in_row++;
}

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
