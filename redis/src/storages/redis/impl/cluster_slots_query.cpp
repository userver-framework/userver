#include <storages/redis/impl/cluster_slots_query.hpp>

#include <algorithm>
#include <optional>

#include <fmt/format.h>
#include <hiredis/hiredis.h>
#include <iostream>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/shard.hpp>
#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

namespace {

std::optional<std::string> GetIpFromMeta(const ReplyData::Array& host_info_array) {
    if (host_info_array.size() < 4) {
        return std::nullopt;
    }
    const auto& meta = host_info_array[3];
    if (!meta.IsArray() || meta.GetSize() < 2) {
        return std::nullopt;
    }

    const auto& array = meta.GetArray();
    if (!array[0].IsString() || !array[1].IsString() || array[0].GetString() != "ip") {
        return std::nullopt;
    }
    return array[1].GetString();
}

std::string GetIpFromHostInfo(const ReplyData::Array& host_info_array) {
    auto from_meta = GetIpFromMeta(host_info_array);
    if (from_meta) {
        return *from_meta;
    }
    return host_info_array[0].GetString();
}

}  // namespace

logging::LogHelper& operator<<(logging::LogHelper& log, SlotInterval interval) {
    log << '[' << interval.slot_min << ',' << interval.slot_max << ']';
    return log;
}

ClusterSlotsResponseStatus ParseClusterSlotsResponse(
    const ReplyPtr& reply,
    ClusterSlotsResponse& res,
    const std::string& shard_group_name
) {
    UASSERT(reply);
    auto log_extra = [&shard_group_name] { return logging::LogExtra({{"shard_group_name", shard_group_name}}); };

    LOG_DEBUG() << "Received CLUSTER SLOTS response: " << reply->data.ToDebugString();

    LOG_TRACE() << log_extra() << "Got reply to CLUSTER SLOTS: " << reply->data.ToDebugString();
    if (reply->IsUnknownCommandError()) {
        LOG_ERROR()
            << log_extra()
            << "Redis CLUSTER SLOTS reply contains unknown command error: " << reply->data.ToDebugString();
        return ClusterSlotsResponseStatus::kNonCluster;
    }
    if (reply->status != ReplyStatus::kOk) {
        LOG_ERROR() << log_extra() << "Redis CLUSTER SLOTS reply contains error: " << reply->data.ToDebugString();
        return ClusterSlotsResponseStatus::kFail;
    }
    if (!reply->data.IsArray()) {
        LOG_ERROR() << log_extra() << "Redis CLUSTER SLOTS reply is not an array: " << reply->data.ToDebugString();
        return ClusterSlotsResponseStatus::kFail;
    }

    std::size_t array_index = 0;
    for (const auto& reply_interval : reply->data.GetArray()) {
        if (!reply_interval.IsArray()) {
            LOG_ERROR()
                << log_extra() << "Redis CLUSTER SLOTS reply element[" << array_index
                << "] is not an array: " << reply_interval.ToDebugString();
            return ClusterSlotsResponseStatus::kFail;
        }

        const auto& array = reply_interval.GetArray();
        if (array.size() < 3) {
            LOG_ERROR()
                << log_extra() << "Redis CLUSTER SLOTS reply element[" << array_index
                << "] is an array of size less than 3: " << reply_interval.ToDebugString();
            return ClusterSlotsResponseStatus::kFail;
        }

        if (!array[0].IsInt()) {
            LOG_ERROR()
                << log_extra() << "Redis CLUSTER SLOTS reply element[" << array_index
                << "][0] is not an int: " << array[0].ToDebugString();
            return ClusterSlotsResponseStatus::kFail;
        }

        if (!array[1].IsInt()) {
            LOG_ERROR()
                << log_extra() << "Redis CLUSTER SLOTS reply element[" << array_index
                << "][1] is not an int: " << array[1].ToDebugString();
            return ClusterSlotsResponseStatus::kFail;
        }

        for (std::size_t i = 2; i < array.size(); i++) {
            if (!array[i].IsArray()) {
                LOG_ERROR()
                    << log_extra() << "Redis CLUSTER SLOTS reply element[" << array_index << "][" << i
                    << "] is not an array: " << array[i].ToDebugString();
                return ClusterSlotsResponseStatus::kFail;
            }

            const auto& host_info_array = array[i].GetArray();
            if (host_info_array.size() < 2) {
                LOG_ERROR()
                    << log_extra() << "Redis CLUSTER SLOTS reply element[" << array_index << "][" << i
                    << "] is an array of size less than 2: " << array[i].ToDebugString();
                return ClusterSlotsResponseStatus::kFail;
            }

            if (!host_info_array[0].IsString()) {
                LOG_ERROR()
                    << log_extra() << "Redis CLUSTER SLOTS reply element[" << array_index << "][" << i
                    << "][0] is not a string: " << host_info_array[0].ToDebugString();
                return ClusterSlotsResponseStatus::kFail;
            }

            if (!host_info_array[1].IsInt()) {
                LOG_ERROR()
                    << log_extra() << "Redis CLUSTER SLOTS reply element[" << array_index << "][" << i
                    << "][1] is not an int: " << host_info_array[1].ToDebugString();
                return ClusterSlotsResponseStatus::kFail;
            }

            ConnectionInfoInt conn_info{
                {GetIpFromHostInfo(host_info_array), static_cast<int>(host_info_array[1].GetInt()), {}}
            };
            const SlotInterval slot_interval(array[0].GetInt(), array[1].GetInt());
            if (i == 2) {
                const bool is_master_overwritten =
                    (!res[slot_interval].master.HostPort().first.empty() &&
                     res[slot_interval].master.HostPort().first != "localhost" &&
                     res[slot_interval].master.HostPort() != conn_info.HostPort());
                if (is_master_overwritten) {
                    const auto message = fmt::format(
                        "Redis CLUSTER SLOTS reply element[{}][{}] overwrites master '{}' with '{}'",
                        array_index,
                        i,
                        res[slot_interval].master.HostPort().first,
                        conn_info.HostPort().first
                    );
                    LOG_ERROR() << log_extra() << message;
                    UASSERT_MSG(false, message);
                }

                res[slot_interval].master = std::move(conn_info);
            } else {
                res[slot_interval].slaves.insert(std::move(conn_info));
            }
        }

        ++array_index;
    }
    return ClusterSlotsResponseStatus::kOk;
}

void GetClusterSlotsContext::ProcessRequest(
    std::shared_ptr<const std::vector<std::string>> shard_names,
    GetClusterSlotsRequest request,
    ProcessGetClusterHostsRequestCb callback
) {
    auto ids = request.sentinel_shard.GetAllInstancesServerId();
    auto context = std::make_shared<GetClusterSlotsContext>(
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

GetClusterSlotsContext::GetClusterSlotsContext(
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

void GetClusterSlotsContext::OnAsyncCommandFailed() {
    --expected_responses_cnt_;

    ProcessResponses();
}

void GetClusterSlotsContext::OnResponse(const CommandPtr&, const ReplyPtr& reply) {
    ClusterSlotsResponse response;
    switch (ParseClusterSlotsResponse(reply, response, shard_group_name_)) {
        case ClusterSlotsResponseStatus::kOk: {
            {
                const std::lock_guard<std::mutex> lock(mutex_);
                responses_by_id_[reply->server_id] = std::move(response);
            }
            responses_parsed_++;
            break;
        }
        case ClusterSlotsResponseStatus::kFail:
            break;
        case ClusterSlotsResponseStatus::kNonCluster:
            is_non_cluster_ = true;
            break;
    }

    response_got_++;

    ProcessResponses();
}

void GetClusterSlotsContext::ProcessResponses() {
    if (response_got_ >= expected_responses_cnt_ || is_non_cluster_) {
        if (!process_responses_started_.test_and_set()) {
            ProcessResponsesOnce();
        }
    }
}

void ProceResponseOnceImpl(
    const std::map<ServerId, ClusterSlotsResponse>& responses_by_id,
    const ProcessGetClusterHostsRequestCb& callback,
    const std::string& shard_group_name,
    const std::vector<std::string>& shard_names,
    const Credentials& credentials,
    size_t expected_responses_cnt,
    size_t responses_parsed,
    bool is_non_cluster
) {
    ClusterShardHostInfos res;
    if (is_non_cluster) {
        callback(res, expected_responses_cnt, responses_parsed, is_non_cluster);
        return;
    }

    auto log_extra = [&shard_group_name] { return logging::LogExtra({{"shard_group_name", shard_group_name}}); };
    std::set<size_t> slot_bounds;
    for (const auto& [_, response] : responses_by_id) {
        for (const auto& [interval, _] : response) {
            slot_bounds.insert(interval.slot_min);
            slot_bounds.insert(interval.slot_max + 1);
        }
    }
    if (slot_bounds.empty()) {
        LOG_WARNING()
            << log_extra() << "Failed to process CLUSTER SLOTS replies: responses_parsed=" << responses_parsed
            << ", no slots info found";
    } else if (*slot_bounds.begin() != 0 || *std::prev(slot_bounds.end()) != kClusterHashSlots) {
        LOG_ERROR()
            << log_extra() << "Failed to process CLUSTER SLOTS replies: slot bounds begin=" << *slot_bounds.begin()
            << ", end=" << *std::prev(slot_bounds.end());
    }

    if (!slot_bounds.empty() && *slot_bounds.begin() == 0 && *std::prev(slot_bounds.end()) == kClusterHashSlots) {
        size_t prev = 0;
        std::map<ConnectionInfoInt, size_t> master_count;
        struct ShardInfo {
            ConnectionInfoInt master;
            std::set<SlotInterval> slot_intervals;
        };
        std::map<std::set<ConnectionInfoInt>, ShardInfo> shard_infos;

        for (const size_t bound : slot_bounds) {
            if (bound) {
                const SlotInterval interval{prev, bound - 1};
                std::map<std::set<ConnectionInfoInt>, size_t> shard_stats;
                for (const auto& [_, response] : responses_by_id) {
                    auto it = response.upper_bound(interval);
                    if (it != response.begin()) {
                        --it;
                        auto hosts = it->second.slaves;
                        hosts.insert(it->second.master);
                        ++shard_stats[hosts];
                        ++master_count[it->second.master];
                    }
                }

                size_t max_count = 0;
                // `best` - most popular set of hosts assigned to the current interval
                // of hash slots among all host replies.
                const std::set<ConnectionInfoInt>* best = nullptr;

                for (const auto& stat : shard_stats) {
                    if (stat.second > max_count) {
                        max_count = stat.second;
                        best = &stat.first;
                    }
                }
                if (best) {
                    shard_infos[*best].slot_intervals.insert(interval);
                }
            }
            prev = bound;
        }

        for (auto& shard_info : shard_infos) {
            size_t max_count = 0;
            const ConnectionInfoInt* master = nullptr;
            for (const auto& host : shard_info.first) {
                const size_t current = master_count[host];
                if (current > max_count) {
                    max_count = current;
                    master = &host;
                }
            }
            UASSERT(master);
            shard_info.second.master = *master;

            ClusterShardHostInfo host_info{shard_info.second.master, {}, {}};
            for (const auto& slave : shard_info.first) {
                if (slave != host_info.master) {
                    host_info.slaves.push_back(slave);
                }
            }
            host_info.slot_intervals = std::move(shard_info.second.slot_intervals);
            res.push_back(std::move(host_info));
        }

        size_t shard_index = 0;
        if (res.size() > shard_names.size()) {
            LOG_ERROR()
                << log_extra() << "Too many shards found: " << res.size() << ", maximum: " << shard_names.size();
            res = {};
        } else {
            std::ranges::sort(res);
            for (auto& shard_host_info : res) {
                shard_host_info.master.SetCredentials(credentials);
                shard_host_info.master.SetName(shard_names[shard_index]);
                for (auto& slave : shard_host_info.slaves) {
                    slave.SetCredentials(credentials);
                    slave.SetName(shard_names[shard_index]);
                }
                ++shard_index;
            }
        }
    }

    callback(res, expected_responses_cnt, responses_parsed, is_non_cluster);
}

void GetClusterSlotsContext::ProcessResponsesOnce() {
    ProceResponseOnceImpl(
        responses_by_id_,
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
