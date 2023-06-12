#include <storages/redis/impl/sentinel_query.hpp>

#include <sstream>

#include <fmt/format.h>
#include <hiredis/hiredis.h>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/sentinel_impl.hpp>
#include <storages/redis/impl/shard.hpp>
#include <userver/storages/redis/impl/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

namespace {

std::set<std::string> SentinelParseFlags(const std::string& flags) {
  std::set<std::string> res;
  size_t l = 0;
  size_t r = 0;

  do {
    r = flags.find(',', l);
    std::string flag = flags.substr(l, r - l);
    if (!flag.empty()) res.insert(flag);
    l = r + 1;
  } while (r != std::string::npos);

  return res;
}

const logging::LogExtra& GetLogExtra(const CommandPtr& command) {
  static const logging::LogExtra kEmptyLogExtra{};
  return (command ? command->log_extra : kEmptyLogExtra);
}

bool ParseSentinelResponse(
    const CommandPtr& command, const ReplyPtr& reply, bool allow_empty,
    std::vector<std::map<std::string, std::string>>& res) {
  const auto& reply_data = reply->data;
  auto status = reply->status;
  res.clear();
  if (!reply_data || status != ReplyStatus::kOk ||
      reply_data.GetType() != ReplyData::Type::kArray ||
      (!allow_empty && reply_data.GetArray().empty())) {
    std::stringstream ss;
    if (status != ReplyStatus::kOk) {
      ss << "request to sentinel failed with status=" << reply->status;
    } else {
      ss << "can't parse sentinel response. type=" << reply_data.GetTypeString()
         << " msg=" << reply_data.ToDebugString();
    }
    ss << " server=" << reply->server;
    LOG_WARNING() << ss.str() << GetLogExtra(command);
    return false;
  }

  for (const ReplyData& reply_in : reply_data.GetArray()) {
    if (reply_in && reply_in.GetType() == ReplyData::Type::kArray &&
        !reply_in.GetArray().empty()) {
      const auto& array = reply_in.GetArray();
      for (const auto& elem : array) {
        if (!elem.IsString()) {
          LOG_ERROR() << "unexpected type of reply array element: "
                      << elem.GetTypeString() << " instead of "
                      << ReplyData::TypeToString(ReplyData::Type::kString)
                      << GetLogExtra(command);
          return false;
        }
      }

      auto& properties = res.emplace_back();
      for (size_t k = 0; k < array.size() - 1; k += 2) {
        properties[array[k].GetString()] = array[k + 1].GetString();
      }
    }
  }

  return true;
}

struct InstanceStatus {
  size_t count_ok = 0;

  size_t o_down_count = 0;
  size_t disconnected_count = 0;
  size_t s_down_count = 0;
  size_t master_link_not_ok_count = 0;
};

void UpdateInstanceStatus(const SentinelInstanceResponse& properties,
                          InstanceStatus& status) {
  try {
    auto ok = true;
    auto flags = SentinelParseFlags(properties.at("flags"));
    bool master = flags.find("master") != flags.end();
    bool slave = flags.find("slave") != flags.end();
    if (master || slave) {
      if (flags.find("s_down") != flags.end()) {
        status.s_down_count++;
        ok = false;
      }
      if (flags.find("o_down") != flags.end()) {
        status.o_down_count++;
        ok = false;
      }
      if (flags.find("disconnected") != flags.end()) {
        status.disconnected_count++;
        ok = false;
      }
      if (slave && properties.at("master-link-status") != "ok") {
        status.master_link_not_ok_count++;
        ok = false;
      }
    }

    if (ok) status.count_ok++;
  } catch (const std::out_of_range& e) {
    LOG_WARNING() << "Failed to handle sentinel reply (system flags): "
                  << e.what();
    // ignore this reply
  }
}

class InstanceUpChecker {
 public:
  InstanceUpChecker(const InstanceStatus& status, size_t sentinel_count);

  std::string GetReason() const;

  bool IsInstanceUp() const { return reason_ == InstanceDownReason::kOk; }

 private:
  enum class InstanceDownReason {
    kOk,
    kSDown,
    kDisconnected,
    kMasterLinkNotOk,
    kODown,
    kTooFewOks,
  };

  const size_t sentinel_count_;
  const size_t quorum_;
  InstanceDownReason reason_{InstanceDownReason::kOk};
  size_t counter_{0};
};

InstanceUpChecker::InstanceUpChecker(const InstanceStatus& status,
                                     size_t sentinel_count)
    : sentinel_count_(sentinel_count), quorum_(sentinel_count / 2 + 1) {
  /* A single sentinel might go crazy and see invalid redis instance state,
   * believe only a quorum of sentinels.
   */
  if (status.s_down_count >= quorum_) {
    reason_ = InstanceDownReason::kSDown;
    counter_ = status.s_down_count;
    return;
  }

  if (status.disconnected_count >= quorum_) {
    reason_ = InstanceDownReason::kDisconnected;
    counter_ = status.disconnected_count;
    return;
  }

  if (status.master_link_not_ok_count >= quorum_) {
    reason_ = InstanceDownReason::kMasterLinkNotOk;
    counter_ = status.master_link_not_ok_count;
    return;
  }

  /* o_down=1 means the sentinel saw enough s_down for a short period of time,
   * let's trust the sentinel.
   */
  if (status.o_down_count > 0) {
    reason_ = InstanceDownReason::kODown;
    counter_ = status.o_down_count;
    return;
  }

  if (status.count_ok < quorum_) {
    reason_ = InstanceDownReason::kTooFewOks;
    counter_ = status.count_ok;
    return;
  }
}

std::string InstanceUpChecker::GetReason() const {
  switch (reason_) {
    case InstanceDownReason::kOk:
      return "Instance is OK";
    case InstanceDownReason::kSDown:
      return fmt::format(
          "Too many sentinel replies with 's_down' flag ({} >= {} of {})",
          counter_, quorum_, sentinel_count_);
    case InstanceDownReason::kDisconnected:
      return fmt::format(
          "Too many sentinel replies with 'disconnected' flag ({} >= {} of {})",
          counter_, quorum_, sentinel_count_);
    case InstanceDownReason::kMasterLinkNotOk:
      return fmt::format(
          "Too many sentinel replies with 'master-link-status' != 'ok' ({} >= "
          "{} of {})",
          counter_, quorum_, sentinel_count_);
    case InstanceDownReason::kODown:
      return fmt::format(
          "Too many sentinel replies with 'o_down' flag ({} > 0 of {})",
          counter_, sentinel_count_);
    case InstanceDownReason::kTooFewOks:
      return fmt::format(
          "Too few sentinels report that host is good ({} < {} of {})",
          counter_, quorum_, sentinel_count_);
  }

  UINVARIANT(false, "Unknown reason");
}

enum class ClusterSlotsResponseStatus {
  kOk,
  kFail,
  kNonCluster,
};

ClusterSlotsResponseStatus ParseClusterSlotsResponse(
    const ReplyPtr& reply, ClusterSlotsResponse& res) {
  UASSERT(reply);
  LOG_TRACE() << "Got reply to CLUSTER SLOTS: " << reply->data.ToDebugString();
  if (reply->IsUnknownCommandError())
    return ClusterSlotsResponseStatus::kNonCluster;
  if (reply->status != ReplyStatus::kOk || !reply->data.IsArray())
    return ClusterSlotsResponseStatus::kFail;
  for (const auto& reply_interval : reply->data.GetArray()) {
    if (!reply_interval.IsArray()) return ClusterSlotsResponseStatus::kFail;
    const auto& array = reply_interval.GetArray();
    if (array.size() < 3) return ClusterSlotsResponseStatus::kFail;
    if (!array[0].IsInt() || !array[1].IsInt())
      return ClusterSlotsResponseStatus::kFail;
    for (size_t i = 2; i < array.size(); i++) {
      if (!array[i].IsArray()) return ClusterSlotsResponseStatus::kFail;
      const auto& host_info_array = array[i].GetArray();
      if (host_info_array.size() < 2) return ClusterSlotsResponseStatus::kFail;
      if (!host_info_array[0].IsString() || !host_info_array[1].IsInt())
        return ClusterSlotsResponseStatus::kFail;
      ConnectionInfoInt conn_info{
          {host_info_array[0].GetString(),
           static_cast<int>(host_info_array[1].GetInt()),
           {}}};
      SlotInterval slot_interval(array[0].GetInt(), array[1].GetInt());
      if (i == 2)
        res[slot_interval].master = std::move(conn_info);
      else
        res[slot_interval].slaves.insert(std::move(conn_info));
    }
  }
  return ClusterSlotsResponseStatus::kOk;
}

}  // namespace

GetHostsContext::GetHostsContext(bool allow_empty, const Password& password,
                                 ProcessGetHostsRequestCb&& callback,
                                 size_t expected_responses_cnt)
    : allow_empty_(allow_empty),
      password_(password),
      callback_(std::move(callback)),
      expected_responses_cnt_(expected_responses_cnt) {}

std::function<void(const CommandPtr&, const ReplyPtr&)>
GetHostsContext::GenerateCallback() {
  return [self = shared_from_this()](const CommandPtr& command,
                                     const ReplyPtr& reply) {
    self->OnResponse(command, reply);
  };
}

void GetHostsContext::OnResponse(const CommandPtr& command,
                                 const ReplyPtr& reply) {
  bool need_process_responses = false;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    response_got_++;

    SentinelResponse response;
    if (ParseSentinelResponse(command, reply, allow_empty_, response)) {
      responses_parsed_++;
      for (auto& instance_response : response) {
        const auto& name = instance_response["name"];
        responses_by_name_[name].push_back(std::move(instance_response));
      }
    }

    if (response_got_ >= expected_responses_cnt_) {
      need_process_responses = !process_responses_started_.test_and_set();
    }
  }
  if (need_process_responses) ProcessResponsesOnce();
}

std::map<std::string, std::vector<SentinelInstanceResponse>>
GroupResponsesByHost(const SentinelResponse& response) {
  std::map<std::string, std::vector<SentinelInstanceResponse>> result;
  for (const auto& r : response) {
    const auto& group_key = r.at("ip") + ':' + r.at("port");
    result[group_key].push_back(r);
  }
  return result;
}

void GetHostsContext::ProcessResponsesOnce() {
  ConnInfoByShard res;

  for (const auto& it : responses_by_name_) {
    const auto by_host = GroupResponsesByHost(it.second);
    for (const auto& group : by_host) {
      InstanceStatus status;
      // UpdateInstanceStatus(group, status);
      for (const auto& response : group.second) {
        UpdateInstanceStatus(response, status);
      }
      const auto& properties = group.second.front();

      try {
        ConnectionInfoInt info{
            {properties.at("ip"), std::stoi(properties.at("port")), password_}};
        info.SetName(properties.at("name"));

        InstanceUpChecker instance_up_checker(status, expected_responses_cnt_);
        if (instance_up_checker.IsInstanceUp()) {
          res.push_back(std::move(info));
        } else {
          const auto host_port = info.HostPort();
          LOG_INFO() << "Skip redis server instance: name=" << info.Name()
                     << ", host=" << host_port.first
                     << ", port=" << host_port.second
                     << ", reason: " << instance_up_checker.GetReason();
        }
      } catch (const std::invalid_argument& e) {
        LOG_WARNING() << "Failed to handle sentinel reply (data): " << e.what();
      } catch (const std::out_of_range& e) {
        LOG_WARNING() << "Failed to handle sentinel reply (data): " << e.what();
      }
    }
  }

  callback_(res, expected_responses_cnt_, responses_parsed_);
}

void ProcessGetHostsRequest(GetHostsRequest request,
                            ProcessGetHostsRequestCb callback) {
  const auto allow_empty = !request.master;

  auto ids = request.sentinel_shard.GetAllInstancesServerId();
  auto context = std::make_shared<GetHostsContext>(
      allow_empty, request.password, std::move(callback), ids.size());

  for (const auto& id : ids) {
    auto cmd =
        PrepareCommand(request.command.Clone(), context->GenerateCallback());
    cmd->control.force_server_id = id;
    request.sentinel_shard.AsyncCommand(cmd);
  }
}

void ProcessGetClusterHostsRequest(
    std::shared_ptr<const std::vector<std::string>> shard_names,
    GetClusterHostsRequest request, ProcessGetClusterHostsRequestCb callback) {
  auto ids = request.sentinel_shard.GetAllInstancesServerId();
  auto context = std::make_shared<GetClusterHostsContext>(
      request.password, std::move(shard_names), std::move(callback),
      ids.size());

  for (const auto& id : ids) {
    auto cmd =
        PrepareCommand(request.command.Clone(), context->GenerateCallback());
    cmd->control.force_server_id = id;
    if (!request.sentinel_shard.AsyncCommand(cmd)) {
      context->OnAsyncCommandFailed();
    }
  }
}

GetClusterHostsContext::GetClusterHostsContext(
    Password password,
    std::shared_ptr<const std::vector<std::string>> shard_names,
    ProcessGetClusterHostsRequestCb&& callback, size_t expected_responses_cnt)
    : password_(std::move(password)),
      shard_names_(std::move(shard_names)),
      callback_(std::move(callback)),
      expected_responses_cnt_(expected_responses_cnt) {}

std::function<void(const CommandPtr&, const ReplyPtr&)>
GetClusterHostsContext::GenerateCallback() {
  return [self = shared_from_this()](const CommandPtr& command,
                                     const ReplyPtr& reply) {
    self->OnResponse(command, reply);
  };
}

void GetClusterHostsContext::OnAsyncCommandFailed() {
  --expected_responses_cnt_;

  ProcessResponses();
}

void GetClusterHostsContext::OnResponse(const CommandPtr&,
                                        const ReplyPtr& reply) {
  ClusterSlotsResponse response;
  switch (ParseClusterSlotsResponse(reply, response)) {
    case ClusterSlotsResponseStatus::kOk: {
      {
        std::unique_lock<std::mutex> lock(mutex_);
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

void GetClusterHostsContext::ProcessResponses() {
  if (response_got_ >= expected_responses_cnt_ || is_non_cluster_) {
    if (!process_responses_started_.test_and_set()) ProcessResponsesOnce();
  }
}

void GetClusterHostsContext::ProcessResponsesOnce() {
  ClusterShardHostInfos res;
  if (is_non_cluster_) {
    callback_(res, expected_responses_cnt_, responses_parsed_, is_non_cluster_);
    return;
  }

  std::set<size_t> slot_bounds;
  for (const auto& [_, response] : responses_by_id_) {
    for (const auto& [interval, _] : response) {
      slot_bounds.insert(interval.slot_min);
      slot_bounds.insert(interval.slot_max + 1);
    }
  }
  if (slot_bounds.empty()) {
    LOG_WARNING()
        << "Failed to process CLUSTER SLOTS replies: responses_parsed="
        << responses_parsed_ << ", no slots info found";
  } else if (*slot_bounds.begin() != 0 ||
             *std::prev(slot_bounds.end()) != kClusterHashSlots) {
    LOG_ERROR() << "Failed to process CLUSTER SLOTS replies: slot bounds begin="
                << *slot_bounds.begin()
                << ", end=" << *std::prev(slot_bounds.end());
  }

  if (!slot_bounds.empty() && *slot_bounds.begin() == 0 &&
      *std::prev(slot_bounds.end()) == kClusterHashSlots) {
    size_t prev = 0;
    std::map<ConnectionInfoInt, size_t> master_count;
    struct ShardInfo {
      ConnectionInfoInt master;
      std::set<SlotInterval> slot_intervals;
    };
    std::map<std::set<ConnectionInfoInt>, ShardInfo> shard_infos;

    for (size_t bound : slot_bounds) {
      if (bound) {
        SlotInterval interval{prev, bound - 1};
        std::map<std::set<ConnectionInfoInt>, size_t> shard_stats;
        for (const auto& [_, response] : responses_by_id_) {
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
        size_t current = master_count[host];
        if (current > max_count) {
          max_count = current;
          master = &host;
        }
      }
      UASSERT(master);
      shard_info.second.master = *master;

      ClusterShardHostInfo host_info{shard_info.second.master, {}, {}};
      for (const auto& slave : shard_info.first) {
        if (slave != host_info.master) host_info.slaves.push_back(slave);
      }
      host_info.slot_intervals = std::move(shard_info.second.slot_intervals);
      res.push_back(std::move(host_info));
    }

    size_t shard_index = 0;
    if (res.size() > shard_names_->size()) {
      LOG_ERROR() << "Too many shards found: " << res.size()
                  << ", maximum: " << shard_names_->size();
      res = {};
    } else {
      std::sort(res.begin(), res.end());
      for (auto& shard_host_info : res) {
        shard_host_info.master.SetPassword(password_);
        shard_host_info.master.SetName((*shard_names_)[shard_index]);
        for (auto& slave : shard_host_info.slaves) {
          slave.SetPassword(password_);
          slave.SetName((*shard_names_)[shard_index]);
        }
        ++shard_index;
      }
    }
  }

  callback_(res, expected_responses_cnt_, responses_parsed_, is_non_cluster_);
}

}  // namespace redis

USERVER_NAMESPACE_END
