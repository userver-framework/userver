#include "cluster_shards_query.hpp"

#include <hiredis/hiredis.h>
#include <iostream>

#include <userver/logging/log.hpp>
#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

namespace {

bool ParseClusterShardsHost(const ReplyData& data, ClusterShardsHost& shard_host, const std::string& shard_group_name) {
    auto log_extra = [&shard_group_name] { return logging::LogExtra({{"shard_group_name", shard_group_name}}); };

    if (!data.IsArray()) {
        LOG_ERROR() << log_extra() << "Expected array, got: " << data.GetTypeString();
        return false;
    }
    const auto& arr = data.GetArray();

    // The response must be a flat array of key/value pairs.
    // If the number of elements is odd, the last key has no value – treat as error.
    if (arr.size() % 2 != 0) {
        LOG_ERROR() << log_extra() << "Odd number of elements in host map";
        return false;
    }

    bool has_ip = false;
    bool has_port = false;
    bool has_role = false;

    for (size_t i = 0; i + 1 < arr.size(); i += 2) {
        const auto& key_elem = arr[i];
        const auto& val_elem = arr[i + 1];

        // Keys must be strings; otherwise the format is invalid.
        if (!key_elem.IsString()) {
            LOG_ERROR() << log_extra() << "Host map key is not a string";
            return false;
        }
        const auto& key = key_elem.GetString();

        if (key == "ip") {
            if (!val_elem.IsString()) {
                LOG_ERROR() << log_extra() << "Host 'ip' value is not a string";
                return false;
            }
            shard_host.ip = val_elem.GetString();
            has_ip = true;
        } else if (key == "hostname") {
            if (!val_elem.IsString()) {
                LOG_ERROR() << log_extra() << "Host 'hostname' value is not a string";
                return false;
            }
            shard_host.hostname = val_elem.GetString();
        } else if (key == "port") {
            if (!val_elem.IsInt()) {
                LOG_ERROR() << log_extra() << "Host 'port' value is not an integer";
                return false;
            }
            shard_host.port = static_cast<uint16_t>(val_elem.GetInt());
            has_port = true;
        } else if (key == "role") {
            if (!val_elem.IsString()) {
                LOG_ERROR() << log_extra() << "Host 'role' value is not a string";
                return false;
            }
            shard_host.is_master = val_elem.GetString() == "master";
            has_role = true;
        } else if (key == "replication-offset") {
            if (!val_elem.IsInt()) {
                LOG_ERROR() << log_extra() << "Host 'replication-offset' value is not an integer";
                return false;
            }
            shard_host.replication_offset = val_elem.GetInt();
        } else if (key == "health") {
            if (!val_elem.IsString()) {
                LOG_ERROR() << log_extra() << "Host 'health' value is not an string";
                return false;
            }
            shard_host.health_online = val_elem.GetString() == "online";
        }
        // Unknown keys are ignored – they do not affect validation.
    }

    if (!has_ip) {
        LOG_ERROR() << log_extra() << "Missing required 'ip' field in host";
    }
    if (!has_port) {
        LOG_ERROR() << log_extra() << "Missing required 'port' field in host";
    }
    if (!has_role) {
        LOG_ERROR() << log_extra() << "Missing required 'role' field in host";
    }

    // A failed master is not considered a real master
    shard_host.is_master = shard_host.is_master && shard_host.health_online;

    return has_ip && has_port && has_role;
}

bool ParseShardNodes(
    const ReplyData& data,
    std::vector<ClusterShardsHost>& nodes,
    const std::string& shard_group_name
) {
    auto log_extra = [&shard_group_name] { return logging::LogExtra({{"shard_group_name", shard_group_name}}); };

    if (!data.IsArray()) {
        LOG_ERROR() << log_extra() << "Expected array for nodes list";
        return false;
    }

    const auto& arr = data.GetArray();

    nodes.clear();
    nodes.reserve(arr.size());
    for (const auto& node_elem : arr) {
        ClusterShardsHost host;
        if (!ParseClusterShardsHost(node_elem, host, shard_group_name)) {
            LOG_ERROR() << log_extra() << "Failed to parse a node entry";
            return false;
        }
        nodes.emplace_back(std::move(host));
    }
    return true;
}

bool ParseSlots(const ReplyData& data, std::vector<SlotInterval>& slot_intervals, const std::string& shard_group_name) {
    auto log_extra = [&shard_group_name] { return logging::LogExtra({{"shard_group_name", shard_group_name}}); };

    if (!data.IsArray()) {
        LOG_ERROR() << log_extra() << "Expected array for slots";
        return false;
    }
    const auto& arr = data.GetArray();

    // The response must be a flat array of key/value pairs.
    // If the number of elements is odd, the last key has no value – treat as error.
    if (arr.size() % 2 != 0) {
        LOG_ERROR() << log_extra() << "Odd number of elements in slots array";
        return false;
    }

    slot_intervals.clear();
    slot_intervals.reserve(arr.size() / 2);
    for (size_t i = 0; i + 1 < arr.size(); i += 2) {
        const auto& min_reply = arr[i];
        const auto& max_reply = arr[i + 1];
        if (!min_reply.IsInt() || !max_reply.IsInt()) {
            LOG_ERROR() << log_extra() << "Slot boundaries are not integers";
            return false;
        }

        const auto min = min_reply.GetInt();
        const auto max = max_reply.GetInt();

        if (min < 0 || min > 16384) {
            LOG_ERROR() << log_extra() << "Slot min out of range: " << min;
            return false;
        }

        if (max < 0 || max > 16384) {
            LOG_ERROR() << log_extra() << "Slot max out of range: " << max;
            return false;
        }

        if (max < min) {
            LOG_ERROR() << log_extra() << "Slot max less than min: " << max << " < " << min;
            return false;
        }

        slot_intervals.emplace_back(min, max);
    }

    return true;
}

bool ParseClusterShardsShard(const ReplyData& data, ClusterShardsShard& shard, const std::string& shard_group_name) {
    auto log_extra = [&shard_group_name] { return logging::LogExtra({{"shard_group_name", shard_group_name}}); };

    if (!data.IsArray()) {
        LOG_ERROR() << log_extra() << "Expected array";
        return false;
    }
    const auto& arr = data.GetArray();

    // The response must be a flat array of key/value pairs.
    // expected keys are slots, nodes and id
    if (arr.size() % 2 != 0) {
        LOG_ERROR() << log_extra() << "Unexpected number of elements in shard map: " << arr.size();
        return false;
    }

    bool has_slots = false;
    bool has_nodes = false;
    for (size_t i = 0; i + 1 < arr.size(); i += 2) {
        const auto& key_elem = arr[i];
        const auto& val_elem = arr[i + 1];

        if (!key_elem.IsString()) {
            LOG_ERROR() << log_extra() << "Shard map key is not a string";
            return false;
        }

        const auto& key = key_elem.GetString();
        if (key == "slots") {
            if (!ParseSlots(val_elem, shard.slot_intervals, shard_group_name)) {
                LOG_ERROR() << log_extra() << "Failed to parse slots for shard";
                return false;
            }
            has_slots = true;
        } else if (key == "nodes") {
            if (!ParseShardNodes(val_elem, shard.hosts, shard_group_name)) {
                LOG_ERROR() << log_extra() << "Failed to parse nodes for shard";
                return false;
            }
            has_nodes = true;
        }
        // other keys (e.g., "id") are ignored
    }

    if (!has_slots) {
        LOG_ERROR() << log_extra() << "Shard missing required 'slots' field";
    }
    if (!has_nodes) {
        LOG_ERROR() << log_extra() << "Shard missing required 'nodes' field";
    }

    return has_slots && has_nodes;
}

bool ParseClusterShards(
    const ReplyData& data,
    std::vector<ClusterShardsShard>& shards,
    const std::string& shard_group_name
) {
    auto log_extra = [&shard_group_name] { return logging::LogExtra({{"shard_group_name", shard_group_name}}); };

    LOG_DEBUG() << "Received CLUSTER SHARDS response: " << data.ToDebugString();

    if (!data.IsArray()) {
        LOG_ERROR() << log_extra() << "Expected array for cluster shards";
        return false;
    }

    const auto& arr = data.GetArray();
    shards.clear();
    shards.reserve(arr.size());
    for (const auto& shard_elem : arr) {
        ClusterShardsShard shard;
        if (!ParseClusterShardsShard(shard_elem, shard, shard_group_name)) {
            LOG_ERROR() << log_extra() << "Failed to parse a shard element";
            return false;
        }
        shards.emplace_back(std::move(shard));
    }
    return true;
}

}  // namespace

ClusterShardsResponseStatus ParseClusterShardsResponse(
    const ReplyPtr& reply,
    ClusterShardsResponse& res,
    const std::string& shard_group_name
) {
    UASSERT(reply);
    if (reply->IsUnknownCommandError()) {
        return ClusterShardsResponseStatus::kNonCluster;
    }
    if (!reply->IsOk()) {
        return ClusterShardsResponseStatus::kFail;
    }
    if (!reply->data.IsArray()) {
        return ClusterShardsResponseStatus::kFail;
    }
    if (!ParseClusterShards(reply->data, res, shard_group_name)) {
        return ClusterShardsResponseStatus::kFail;
    }

    return ClusterShardsResponseStatus::kOk;
}

GetClusterShardsRequest::GetClusterShardsRequest(
    Shard& sentinel_shard,
    Credentials credentials,
    std::string shard_group_name
)
    : sentinel_shard(sentinel_shard),
      command({"CLUSTER", "SHARDS"}),
      credentials(std::move(credentials)),
      shard_group_name(std::move(shard_group_name))
{}

void GetClusterShardsContext::ProcessRequest(
    std::shared_ptr<const std::vector<std::string>> shard_names,
    GetClusterShardsRequest request,
    ProcessGetClusterHostsRequestCb callback
) {
    auto ids = request.sentinel_shard.GetAllInstancesServerId();
    auto context = std::make_shared<GetClusterShardsContext>(
        request.credentials,
        std::move(shard_names),
        request.shard_group_name,
        std::move(callback),
        ids.size()
    );
    for (const auto& id : ids) {
        auto cmd = PrepareCommand(request.command.Clone(), [context](const CommandPtr& command, const ReplyPtr& reply) {
            context->OnResponse(command, reply);
        });
        cmd->control.force_server_id = id;
        if (!request.sentinel_shard.AsyncCommand(cmd)) {
            context->OnAsyncCommandFailed();
        }
    }
}

GetClusterShardsContext::GetClusterShardsContext(
    Credentials credentials,
    std::shared_ptr<const std::vector<std::string>> shard_names,
    std::string shard_group_name,
    ProcessGetClusterHostsRequestCb&& callback,
    size_t expected_responses_cnt
)
    : shard_group_name_(std::move(shard_group_name)),
      credentials_(std::move(credentials)),
      shard_names_(std::move(shard_names)),
      callback_(std::move(callback)),
      expected_responses_cnt_(expected_responses_cnt)
{}

void GetClusterShardsContext::OnAsyncCommandFailed() {
    --expected_responses_cnt_;
    ProcessResponses();
}

void GetClusterShardsContext::OnResponse(const CommandPtr&, const ReplyPtr& reply) {
    ClusterShardsResponse response;
    switch (ParseClusterShardsResponse(reply, response, shard_group_name_)) {
        case ClusterShardsResponseStatus::kOk: {
            {
                const std::lock_guard<std::mutex> lock(mutex_);
                responses_by_id_[reply->server_id] = std::move(response);
            }
            ++responses_parsed_;
            break;
        }
        case ClusterShardsResponseStatus::kFail:
            break;
        case ClusterShardsResponseStatus::kNonCluster:
            is_non_cluster_ = true;
            break;
    }
    ++response_got_;
    ProcessResponses();
}

void GetClusterShardsContext::ProcessResponses() {
    if (response_got_ >= expected_responses_cnt_ || is_non_cluster_) {
        if (!process_responses_started_.test_and_set()) {
            ProcessResponsesOnce();
        }
    }
}

std::map<ServerId, ClusterSlotsResponse> ConvertToClusterSlotsResponse(
    const std::unordered_map<ServerId, ClusterShardsResponse, ServerIdHasher>& responses_by_id
) {
    std::map<ServerId, ClusterSlotsResponse> result;
    for (const auto& [server_id, shards] : responses_by_id) {
        ClusterSlotsResponse slots_response;
        for (const auto& shard : shards) {
            // Find master host
            const ClusterShardsHost* master_host = nullptr;
            for (const auto& host : shard.hosts) {
                if (host.is_master) {
                    master_host = &host;
                    break;
                }
            }
            if (!master_host) {
                // No master in this shard – skip it
                LOG_DEBUG() << "No master in shard";
                continue;
            }

            // Build ConnectionInfoInt for master
            MasterSlavesConnInfos msci;
            msci.master = ConnectionInfoInt{{master_host->ip, static_cast<int>(master_host->port), {}}};
            // Add slaves
            for (const auto& host : shard.hosts) {
                if (host.is_master) {
                    continue;
                }
                ConnectionInfoInt slave_conn{{host.ip, static_cast<int>(host.port), {}}};
                msci.slaves.insert(std::move(slave_conn));
            }
            // Associate each slot interval with the master/slaves info
            for (const auto& interval : shard.slot_intervals) {
                slots_response.emplace(interval, msci);
            }
        }
        result.emplace(server_id, std::move(slots_response));
    }
    return result;
}

void GetClusterShardsContext::ProcessResponsesOnce() {
    std::map<ServerId, ClusterSlotsResponse>
        cluster_slots_response = ConvertToClusterSlotsResponse(this->responses_by_id_);
    ProceResponseOnceImpl(
        cluster_slots_response,
        callback_,
        shard_group_name_,
        *shard_names_,
        credentials_,
        expected_responses_cnt_.load(),
        responses_parsed_.load(),
        is_non_cluster_.load()
    );
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
