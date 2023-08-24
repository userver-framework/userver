#pragma once

#include <atomic>

#include <userver/storages/redis/impl/base.hpp>
#include "shard.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

struct GetHostsRequest {
  // For MASTERS
  GetHostsRequest(Shard& sentinel_shard, Password password)
      : sentinel_shard(sentinel_shard),
        command({"SENTINEL", "MASTERS"}),
        master(true),
        password(std::move(password)) {}

  // For SLAVES
  GetHostsRequest(Shard& sentinel_shard, std::string shard_name,
                  Password password)
      : sentinel_shard(sentinel_shard),
        command({"SENTINEL", "SLAVES", std::move(shard_name)}),
        master(false),
        password(std::move(password)) {}

  Shard& sentinel_shard;
  CmdArgs command;

  bool master;
  Password password;
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
  GetHostsContext(bool allow_empty, const Password& password,
                  ProcessGetHostsRequestCb&& callback,
                  size_t expected_responses_cnt);

  std::function<void(const CommandPtr&, const ReplyPtr& reply)>
  GenerateCallback();

 private:
  void OnResponse(const CommandPtr&, const ReplyPtr& reply);
  void ProcessResponsesOnce();

  std::mutex mutex_;
  const bool allow_empty_;
  const Password password_;
  const ProcessGetHostsRequestCb callback_;
  size_t response_got_{0};
  size_t responses_parsed_{0};
  std::atomic_flag process_responses_started_ ATOMIC_FLAG_INIT;
  const size_t expected_responses_cnt_{0};

  std::map<std::string, SentinelResponse> responses_by_name_;
};

static constexpr size_t kClusterHashSlots = 16384;

struct GetClusterHostsRequest {
  GetClusterHostsRequest(Shard& sentinel_shard, Password password)
      : sentinel_shard(sentinel_shard),
        command({"CLUSTER", "SLOTS"}),
        password(std::move(password)) {}

  Shard& sentinel_shard;
  CmdArgs command;

  Password password;
};

struct SlotInterval {
  size_t slot_min;
  size_t slot_max;

  SlotInterval(size_t slot_min, size_t slot_max)
      : slot_min(slot_min), slot_max(slot_max) {}

  bool operator<(const SlotInterval& r) const { return slot_min < r.slot_min; }
  bool operator==(const SlotInterval& r) const {
    return slot_min == r.slot_min && slot_max == r.slot_max;
  }
};

struct ClusterShardHostInfo {
  ConnectionInfoInt master;
  std::vector<ConnectionInfoInt> slaves;
  std::set<SlotInterval> slot_intervals;

  bool operator<(const ClusterShardHostInfo& r) const {
    UASSERT(!slot_intervals.empty());
    UASSERT(!r.slot_intervals.empty());
    return slot_intervals < r.slot_intervals;
  }
};

using ClusterShardHostInfos = std::vector<ClusterShardHostInfo>;

using ProcessGetClusterHostsRequestCb =
    std::function<void(ClusterShardHostInfos shard_infos, size_t requests_sent,
                       size_t responses_parsed, bool is_non_cluster_error)>;

void ProcessGetClusterHostsRequest(
    std::shared_ptr<const std::vector<std::string>> shard_names,
    GetClusterHostsRequest request, ProcessGetClusterHostsRequestCb callback);

struct MasterSlavesConnInfos {
  ConnectionInfoInt master;
  std::set<ConnectionInfoInt> slaves;
};

using ClusterSlotsResponse = std::map<SlotInterval, MasterSlavesConnInfos>;

class GetClusterHostsContext
    : public std::enable_shared_from_this<GetClusterHostsContext> {
 public:
  GetClusterHostsContext(
      Password password,
      std::shared_ptr<const std::vector<std::string>> shard_names,
      ProcessGetClusterHostsRequestCb&& callback,
      size_t expected_responses_cnt);

  std::function<void(const CommandPtr&, const ReplyPtr& reply)>
  GenerateCallback();

  void OnAsyncCommandFailed();

 private:
  void OnResponse(const CommandPtr&, const ReplyPtr& reply);
  void ProcessResponses();
  void ProcessResponsesOnce();

  const Password password_;
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

}  // namespace redis

USERVER_NAMESPACE_END
