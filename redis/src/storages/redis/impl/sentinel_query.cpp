#include <storages/redis/impl/sentinel_query.hpp>

#include <optional>
#include <sstream>

#include <fmt/format.h>
#include <hiredis/hiredis.h>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/shard.hpp>
#include <userver/storages/redis/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

namespace {

std::set<std::string> SentinelParseFlags(const std::string& flags) {
    std::set<std::string> res;
    size_t l = 0;
    size_t r = 0;

    do {
        r = flags.find(',', l);
        const std::string flag = flags.substr(l, r - l);
        if (!flag.empty()) {
            res.insert(flag);
        }
        l = r + 1;
    } while (r != std::string::npos);

    return res;
}

const logging::LogExtra& GetLogExtra(const CommandPtr& command) {
    return (command ? command->GetLogExtra() : logging::kEmptyLogExtra);
}

bool ParseSentinelResponse(
    const CommandPtr& command,
    const ReplyPtr& reply,
    bool allow_empty,
    std::vector<std::map<std::string, std::string>>& res
) {
    const auto& reply_data = reply->data;
    auto status = reply->status;
    res.clear();
    if (!reply_data || status != ReplyStatus::kOk || reply_data.GetType() != ReplyData::Type::kArray ||
        (!allow_empty && reply_data.GetArray().empty()))
    {
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
        if (reply_in && reply_in.GetType() == ReplyData::Type::kArray && !reply_in.GetArray().empty()) {
            const auto& array = reply_in.GetArray();
            for (const auto& elem : array) {
                if (!elem.IsString()) {
                    LOG_ERROR()
                        << "unexpected type of reply array element: " << elem.GetTypeString() << " instead of "
                        << ReplyData::TypeToString(ReplyData::Type::kString) << GetLogExtra(command);
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

void UpdateInstanceStatus(const SentinelInstanceResponse& properties, InstanceStatus& status) {
    try {
        auto ok = true;
        auto flags = SentinelParseFlags(properties.at("flags"));
        const bool master = flags.find("master") != flags.end();
        const bool slave = flags.find("slave") != flags.end();
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

        if (ok) {
            status.count_ok++;
        }
    } catch (const std::out_of_range& e) {
        LOG_WARNING() << "Failed to handle sentinel reply (system flags): " << e.what();
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

InstanceUpChecker::InstanceUpChecker(const InstanceStatus& status, size_t sentinel_count)
    : sentinel_count_(sentinel_count),
      quorum_(sentinel_count / 2 + 1)
{
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
                counter_,
                quorum_,
                sentinel_count_
            );
        case InstanceDownReason::kDisconnected:
            return fmt::format(
                "Too many sentinel replies with 'disconnected' flag ({} >= {} of {})",
                counter_,
                quorum_,
                sentinel_count_
            );
        case InstanceDownReason::kMasterLinkNotOk:
            return fmt::format(
                "Too many sentinel replies with 'master-link-status' != 'ok' ({} >= "
                "{} of {})",
                counter_,
                quorum_,
                sentinel_count_
            );
        case InstanceDownReason::kODown:
            return fmt::format(
                "Too many sentinel replies with 'o_down' flag ({} > 0 of {})",
                counter_,
                sentinel_count_
            );
        case InstanceDownReason::kTooFewOks:
            return fmt::format(
                "Too few sentinels report that host is good ({} < {} of {})",
                counter_,
                quorum_,
                sentinel_count_
            );
    }

    UINVARIANT(false, "Unknown reason");
}
}  // namespace

GetHostsContext::GetHostsContext(
    bool allow_empty,
    const Credentials& credentials,
    ProcessGetHostsRequestCb&& callback,
    size_t expected_responses_cnt
)
    : allow_empty_(allow_empty),
      credentials_(credentials),
      callback_(std::move(callback)),
      expected_responses_cnt_(expected_responses_cnt)
{}

std::function<void(const CommandPtr&, const ReplyPtr&)> GetHostsContext::GenerateCallback() {
    return [self = shared_from_this()](const CommandPtr& command, const ReplyPtr& reply) {
        self->OnResponse(command, reply);
    };
}

void GetHostsContext::OnResponse(const CommandPtr& command, const ReplyPtr& reply) {
    bool need_process_responses = false;
    {
        const std::lock_guard<std::mutex> lock(mutex_);
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
    if (need_process_responses) {
        ProcessResponsesOnce();
    }
}

std::map<std::string, std::vector<SentinelInstanceResponse>> GroupResponsesByHost(const SentinelResponse& response) {
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
                ConnectionInfoInt info{{properties.at("ip"), std::stoi(properties.at("port")), credentials_}};
                info.SetName(properties.at("name"));

                const InstanceUpChecker instance_up_checker(status, expected_responses_cnt_);
                if (instance_up_checker.IsInstanceUp()) {
                    res.push_back(std::move(info));
                } else {
                    const auto host_port = info.HostPort();
                    LOG_INFO()
                        << "Skip redis server instance: name=" << info.Name() << ", host=" << host_port.first
                        << ", port=" << host_port.second << ", reason: " << instance_up_checker.GetReason();
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

void ProcessGetHostsRequest(GetHostsRequest request, ProcessGetHostsRequestCb callback) {
    const auto allow_empty = !request.master;

    auto ids = request.sentinel_shard.GetAllInstancesServerId();
    auto context = std::make_shared<GetHostsContext>(allow_empty, request.credentials, std::move(callback), ids.size());

    for (const auto& id : ids) {
        auto cmd = PrepareCommand(request.command.Clone(), context->GenerateCallback());
        cmd->control.force_server_id = id;
        request.sentinel_shard.AsyncCommand(cmd);
    }
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
