#include "sentinel_query.hpp"

#include <storages/redis/impl/reply.hpp>
#include "sentinel_impl.hpp"
#include "shard.hpp"

namespace ph = std::placeholders;

namespace redis {

namespace {

std::set<std::string> SentinelParseFlags(const std::string& flags) {
  std::set<std::string> res;
  size_t l = 0, r;

  do {
    r = flags.find(',', l);
    std::string flag = flags.substr(l, r - l);
    if (!flag.empty()) res.insert(flag);
    l = r + 1;
  } while (r != std::string::npos);

  return res;
}

bool ParseSentinelResponse(
    const CommandPtr& command, const ReplyPtr& reply, bool allow_empty,
    std::vector<std::map<std::string, std::string>>& res) {
  const auto& reply_data = reply->data;
  int status = reply->status;
  res.clear();
  if (!reply_data || status != REDIS_OK ||
      reply_data.GetType() != ReplyData::Type::kArray ||
      (!allow_empty && reply_data.GetArray().empty())) {
    std::stringstream ss;
    if (status != REDIS_OK) {
      ss << "request to sentinel failed with status=" << reply->StatusString();
    } else {
      ss << "can't parse sentinel response. type=" << reply_data.GetTypeString()
         << " msg=" << reply_data.ToDebugString();
    }
    ss << " server=" << reply->server;
    LOG_WARNING() << ss.str() << command->log_extra;
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
                      << command->log_extra;
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

bool IsInstanceUp(const InstanceStatus& status, size_t sentinel_count) {
  const size_t quorum = sentinel_count / 2 + 1;

  /* A single sentinel might go crazy and see invalid redis instance state,
   * believe only a quorum of sentinels.
   */
  if (status.s_down_count >= quorum) return false;
  if (status.disconnected_count >= quorum) return false;
  if (status.master_link_not_ok_count >= quorum) return false;

  /* o_down=1 means the sentinel saw enough s_down for a short period of time,
   * let's trust the sentinel.
   */
  if (status.o_down_count > 0) return false;

  return status.count_ok >= quorum;
}

}  // namespace

GetHostsContext::GetHostsContext(bool allow_empty, const std::string& password,
                                 ProcessGetHostsRequestCb&& callback,
                                 size_t response_max)
    : allow_empty_(allow_empty),
      password_(password),
      callback_(std::move(callback)),
      response_max_(response_max) {}

std::function<void(const CommandPtr&, const ReplyPtr& reply)>
GetHostsContext::GenerateCallback() {
  return std::bind(&GetHostsContext::OnResponse, shared_from_this(), ph::_1,
                   ph::_2);
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

    if (response_got_ >= response_max_) {
      if (!process_responses_started_) {
        process_responses_started_ = true;
        need_process_responses = true;
      }
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

      if (IsInstanceUp(status, response_max_)) {
        const auto& properties = group.second.front();

        try {
          ConnectionInfoInt info;
          info.name = properties.at("name");
          info.host = properties.at("ip");
          info.port = std::stol(properties.at("port"));
          info.password = password_;
          res.push_back(info);
        } catch (const std::invalid_argument& e) {
          LOG_WARNING() << "Failed to handle sentinel reply (data): "
                        << e.what();
        } catch (const std::out_of_range& e) {
          LOG_WARNING() << "Failed to handle sentinel reply (data): "
                        << e.what();
        }
      }
    }
  }

  callback_(res, response_max_, responses_parsed_);
}

void ProcessGetHostsRequest(GetHostsRequest request,
                            ProcessGetHostsRequestCb callback) {
  const auto allow_empty_ = !request.master;

  auto ids = request.sentinel_shard.GetAllInstancesServerId();
  auto context = std::make_shared<GetHostsContext>(
      allow_empty_, request.password, std::move(callback), ids.size());

  for (const auto& id : ids) {
    auto cmd =
        PrepareCommand(request.command.Clone(), context->GenerateCallback());
    cmd->control.force_server_id = id;
    request.sentinel_shard.AsyncCommand(cmd);
  }
}

}  // namespace redis
