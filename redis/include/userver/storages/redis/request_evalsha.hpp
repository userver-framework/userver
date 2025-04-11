#pragma once

/// @file
/// @brief @copybrief storages::redis::RequestEvalSha

#include <userver/storages/redis/parse_reply.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/storages/redis/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

template <typename ReplyType>
class EvalShaResult;

/// @brief Redis future for EVALSHA responses.
template <typename ScriptResult, typename ReplyType = ScriptResult>
class [[nodiscard]] RequestEvalSha final {
public:
    using EvalShaResult = storages::redis::EvalShaResult<ReplyType>;

    explicit RequestEvalSha(RequestEvalShaCommon&& request) : request_(std::move(request)) {}

    /// Wait for the request to finish on Redis server
    void Wait() { request_.Wait(); }

    /// Ignore the query result and do not wait for the Redis server to finish executing it
    void IgnoreResult() const { request_.IgnoreResult(); }

    /// Wait for the request to finish on Redis server and get the result
    EvalShaResult Get(const std::string& request_description = {}) {
        auto reply_ptr = request_.GetRaw();
        const auto& reply_data = reply_ptr->data;
        if (reply_data.IsError()) {
            const auto& msg = reply_data.GetError();
            if (msg.find("NOSCRIPT", 0) == 0) {
                return EvalShaResult(true);
            }
        }
        /// no error try treat as usual eval
        return impl::ParseReply<ScriptResult, ReplyType>(std::move(reply_ptr), request_description);
    }

private:
    RequestEvalShaCommon request_;
};

/// @brief Value, returned by storages::redis::RequestEvalSha.
template <typename ReplyType>
class EvalShaResult final {
public:
    /// @return true iff the script does not exist on the Redis shard
    bool IsNoScriptError() const noexcept { return no_script_error_; }

    /// @return true iff the script does exist on the Redis shard and returned value
    bool HasValue() const noexcept { return reply_.has_value(); }

    /// Retrieve the value or throw an exception if there's no value
    const ReplyType& Get() const { return reply_.value(); }

    /// Retrieve the value or throws an exception if there's no value
    ReplyType Extract() { return std::move(reply_).value(); }

private:
    template <typename, typename>
    friend class RequestEvalSha;

    EvalShaResult() = default;
    explicit EvalShaResult(bool no_script) : no_script_error_{no_script} {}
    EvalShaResult(ReplyType&& reply) : reply_(std::move(reply)) {}
    std::optional<ReplyType> reply_;
    bool no_script_error_ = false;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
