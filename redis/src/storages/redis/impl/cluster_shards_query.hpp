#pragma once

#include "cluster_slots_query.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

struct GetClusterShardsRequest {
    GetClusterShardsRequest(Shard& sentinel_shard, Credentials credentials, std::string shard_group_name);

    Shard& sentinel_shard;
    CmdArgs command;

    Credentials credentials;
    std::string shard_group_name;
};

enum class ClusterShardsHostHealth { kOnline, kFail, kLoading };

struct ClusterShardsHost {
    std::string ip;
    std::optional<std::string> hostname;
    int64_t replication_offset{0};
    uint16_t port{6379};
    bool is_master{false};
    bool health_online{false};
};

struct ClusterShardsShard {
    std::vector<ClusterShardsHost> hosts;
    std::vector<SlotInterval> slot_intervals;
};

using ClusterShardsResponse = std::vector<ClusterShardsShard>;

class GetClusterShardsContext {
public:
    GetClusterShardsContext(
        Credentials credentials,
        std::shared_ptr<const std::vector<std::string>> shard_names,
        std::string shard_group_name,
        ProcessGetClusterHostsRequestCb&& callback,
        size_t expected_responses_cnt
    );

    static void ProcessRequest(
        std::shared_ptr<const std::vector<std::string>> shard_names,
        GetClusterShardsRequest request,
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
    std::unordered_map<ServerId, ClusterShardsResponse, ServerIdHasher> responses_by_id_;
};

std::map<ServerId, ClusterSlotsResponse> ConvertToClusterSlotsResponse(
    const std::unordered_map<ServerId, ClusterShardsResponse, ServerIdHasher>& responses_by_id
);

enum class ClusterShardsResponseStatus {
    kOk,
    kFail,
    kNonCluster,
};

ClusterShardsResponseStatus ParseClusterShardsResponse(
    const ReplyPtr& reply,
    ClusterShardsResponse& res,
    const std::string& shard_group_name
);

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
