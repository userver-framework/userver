#pragma once

/// @file
/// @brief @copybrief storages::redis::RequestEval

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/storages/redis/parse_reply.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/storages/redis/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief Redis future for EVAL responses.
template <typename ScriptResult, typename ReplyType = ScriptResult>
class [[nodiscard]] RequestEval final {
public:
    explicit RequestEval(RequestEvalCommon&& request)
        : request_(std::move(request))
    {}

    /// Wait for the request to finish on Redis server
    void Wait() { request_.Wait(); }

    /// Ignore the query result and do not wait for the Redis server to finish executing it
    void IgnoreResult() const { request_.IgnoreResult(); }

    /// Wait for the request to finish on Redis server and get the result
    ReplyType Get(const std::string& request_description = {}) {
        return impl::ParseReply<ScriptResult, ReplyType>(request_.GetRaw(), request_description);
    }

    /// Satisfies @ref engine::Awaitable, for use with @ref engine::WaitAnyContext and friends.
    engine::AwaitableToken GetAwaitableToken() noexcept USERVER_IMPL_LIFETIME_BOUND {
        return request_.GetAwaitableToken();
    }

private:
    RequestEvalCommon request_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
