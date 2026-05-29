#pragma once

#include <atomic>
#include <compare>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include <userver/storages/redis/base.hpp>
#include <userver/utils/assert.hpp>

#include "shard.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

static constexpr size_t kClusterHashSlots = 16384;

struct GetClusterSlotsRequest {
    GetClusterSlotsRequest(Shard& sentinel_shard, Credentials credentials, std::string shard_group_name)
        : sentinel_shard(sentinel_shard),
          command({"CLUSTER", "SLOTS"}),
          credentials(std::move(credentials)),
          shard_group_name(std::move(shard_group_name))
    {}

    Shard& sentinel_shard;
    CmdArgs command;

    Credentials credentials;
    std::string shard_group_name;
};

struct SlotInterval {
    size_t slot_min;
    size_t slot_max;

    SlotInterval(size_t slot_min, size_t slot_max)
        : slot_min(slot_min),
          slot_max(slot_max)
    {}

    constexpr auto operator<=>(const SlotInterval&) const = default;
};

logging::LogHelper& operator<<(logging::LogHelper& log, SlotInterval interval);

struct ClusterShardHostInfo {
    ConnectionInfoInt master;
    std::vector<ConnectionInfoInt> slaves;
    std::set<SlotInterval> slot_intervals;

    auto operator<=>(const ClusterShardHostInfo& r) const {
        UASSERT(!slot_intervals.empty());
        UASSERT(!r.slot_intervals.empty());
        return slot_intervals <=> r.slot_intervals;
    }

    bool operator==(const ClusterShardHostInfo& r) const noexcept {
        UASSERT(!slot_intervals.empty());
        UASSERT(!r.slot_intervals.empty());
        return slot_intervals == r.slot_intervals;
    }
};

using ClusterShardHostInfos = std::vector<ClusterShardHostInfo>;

using ProcessGetClusterHostsRequestCb = std::function<
    void(ClusterShardHostInfos shard_infos, size_t requests_sent, size_t responses_parsed, bool is_non_cluster_error)>;

struct MasterSlavesConnInfos {
    ConnectionInfoInt master;
    std::set<ConnectionInfoInt> slaves;
};

using ClusterSlotsResponse = std::map<SlotInterval, MasterSlavesConnInfos>;

class GetClusterSlotsContext {
public:
    GetClusterSlotsContext(
        Credentials credentials,
        std::shared_ptr<const std::vector<std::string>> shard_names,
        std::string shard_group_name,
        ProcessGetClusterHostsRequestCb&& callback,
        size_t expected_responses_cnt
    );

    static void ProcessRequest(
        std::shared_ptr<const std::vector<std::string>> shard_names,
        GetClusterSlotsRequest request,
        ProcessGetClusterHostsRequestCb callback
    );

private:
    void OnAsyncCommandFailed();
    void OnResponse(const CommandPtr&, const ReplyPtr& reply);
    void ProcessResponses();
    void ProcessResponsesOnce();

    const std::string shard_group_name_;
    const Credentials credentials_;
    const std::shared_ptr<const std::vector<std::string>> shard_names_;
    const ProcessGetClusterHostsRequestCb callback_;
    std::atomic<size_t> response_got_{0};
    std::atomic<size_t> responses_parsed_{0};
    std::atomic_flag process_responses_started_ ATOMIC_FLAG_INIT;
    std::atomic<size_t> expected_responses_cnt_{0};
    std::atomic<bool> is_non_cluster_{false};

    std::mutex mutex_;
    std::map<ServerId, ClusterSlotsResponse> responses_by_id_;
};

void ProceResponseOnceImpl(
    const std::map<ServerId, ClusterSlotsResponse>& responses_by_id,
    const ProcessGetClusterHostsRequestCb& callback,
    const std::string& shard_group_name,
    const std::vector<std::string>& shard_names,
    const Credentials& credentials,
    size_t expected_responses_cnt,
    size_t responses_parsed,
    bool is_non_cluster
);

enum class ClusterSlotsResponseStatus {
    kOk,
    kFail,
    kNonCluster,
};

ClusterSlotsResponseStatus ParseClusterSlotsResponse(
    const ReplyPtr& reply,
    ClusterSlotsResponse& res,
    const std::string& shard_group_name
);

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
