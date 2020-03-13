#pragma once

#include <storages/redis/impl/base.hpp>
#include "shard.hpp"

namespace redis {

struct GetHostsRequest {
  // For MASTERS
  GetHostsRequest(Shard& sentinel_shard, std::string password)
      : sentinel_shard(sentinel_shard),
        command({"SENTINEL", "MASTERS"}),
        master(true),
        password(std::move(password)) {}

  // For SLAVES
  GetHostsRequest(Shard& sentinel_shard, std::string shard_name,
                  std::string password)
      : sentinel_shard(sentinel_shard),
        command({"SENTINEL", "SLAVES", std::move(shard_name)}),
        master(false),
        password(std::move(password)) {}

  Shard& sentinel_shard;
  CmdArgs command;

  bool master;
  std::string password;
};

using ProcessGetHostsRequestCb =
    std::function<void(const ConnInfoByShard& info, size_t requests_sent,
                       size_t responses_parsed)>;

void ProcessGetHostsRequest(GetHostsRequest request,
                            ProcessGetHostsRequestCb callback);

using SentinelInstanceResponse = std::map<std::string, std::string>;

using SentinelResponse = std::vector<SentinelInstanceResponse>;

class GetHostsContext : public std::enable_shared_from_this<GetHostsContext> {
 public:
  GetHostsContext(bool allow_empty, const std::string& password,
                  ProcessGetHostsRequestCb&& callback, size_t response_max);

  std::function<void(const CommandPtr&, const ReplyPtr& reply)>
  GenerateCallback();

 private:
  void OnResponse(const CommandPtr&, const ReplyPtr& reply);
  void ProcessResponsesOnce();

 private:
  std::mutex mutex_;
  const bool allow_empty_;
  const std::string password_;
  const ProcessGetHostsRequestCb callback_;
  size_t response_got_{0};
  size_t responses_parsed_{0};
  bool process_responses_started_{false};
  const size_t response_max_{0};

  std::map<std::string, SentinelResponse> responses_by_name_;
};

}  // namespace redis
