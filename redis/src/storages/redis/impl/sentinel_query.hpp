#pragma once

#include <atomic>
#include <set>

#include <userver/storages/redis/base.hpp>
#include <userver/utils/assert.hpp>
#include "shard.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

struct GetHostsRequest {
    static GetHostsRequest QuerySentinelMasters(Shard& sentinel_shard, Credentials credentials) {
        return GetHostsRequest(sentinel_shard, std::move(credentials));
    }
    static GetHostsRequest QuerySentinelSlaves(Shard& sentinel_shard, std::string shard_name, Credentials credentials) {
        return GetHostsRequest(sentinel_shard, std::move(shard_name), std::move(credentials));
    }

private:
    // For MASTERS
    GetHostsRequest(Shard& sentinel_shard, Credentials credentials)
        : sentinel_shard(sentinel_shard),
          command({"SENTINEL", "MASTERS"}),
          master(true),
          credentials(std::move(credentials))
    {
        UASSERT(command.GetCommandCount() == 1);
        UASSERT_MSG(
            fmt::to_string(command.begin()->GetJoinedArgs(";")) == "SENTINEL;MASTERS",
            fmt::to_string(command.begin()->GetJoinedArgs(";"))
        );
    }

    // For SLAVES
    GetHostsRequest(Shard& sentinel_shard, std::string shard_name, Credentials credentials)
        : sentinel_shard(sentinel_shard),
          command({"SENTINEL", "SLAVES", std::move(shard_name)}),
          master(false),
          credentials(std::move(credentials))
    {
        UASSERT(command.GetCommandCount() == 1);
    }

public:
    Shard& sentinel_shard;
    CmdArgs command;

    bool master;
    Credentials credentials;
};

using ProcessGetHostsRequestCb = std::function<
    void(const ConnInfoByShard& info, size_t requests_sent, size_t responses_parsed)>;

void ProcessGetHostsRequest(GetHostsRequest request, ProcessGetHostsRequestCb callback);

using SentinelInstanceResponse = std::map<std::string, std::string>;

using SentinelResponse = std::vector<SentinelInstanceResponse>;

class GetHostsContext : public std::enable_shared_from_this<GetHostsContext> {
public:
    GetHostsContext(
        bool allow_empty,
        const Credentials& credentials,
        ProcessGetHostsRequestCb&& callback,
        size_t expected_responses_cnt
    );

    std::function<void(const CommandPtr&, const ReplyPtr& reply)> GenerateCallback();

private:
    void OnResponse(const CommandPtr&, const ReplyPtr& reply);
    void ProcessResponsesOnce();

    std::mutex mutex_;
    const bool allow_empty_;
    const Credentials credentials_;
    const ProcessGetHostsRequestCb callback_;
    size_t response_got_{0};
    size_t responses_parsed_{0};
    std::atomic_flag process_responses_started_ ATOMIC_FLAG_INIT;
    const size_t expected_responses_cnt_{0};

    std::map<std::string, SentinelResponse> responses_by_name_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
