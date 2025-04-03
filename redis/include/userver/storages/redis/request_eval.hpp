#pragma once

/// @file
/// @brief @copybrief storages::redis::RequestEval

#include <userver/storages/redis/parse_reply.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/storages/redis/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief Redis future for EVAL responses.
template <typename ScriptResult, typename ReplyType = ScriptResult>
class [[nodiscard]] RequestEval final {
public:
    explicit RequestEval(RequestEvalCommon&& request) : request_(std::move(request)) {}

    /// Wait for the request to finish on Redis server
    void Wait() { request_.Wait(); }

    /// Ignore the query result and do not wait for the Redis server to finish executing it
    void IgnoreResult() const { request_.IgnoreResult(); }

    /// Wait for the request to finish on Redis server and get the result
    ReplyType Get(const std::string& request_description = {}) {
        return impl::ParseReply<ScriptResult, ReplyType>(request_.GetRaw(), request_description);
    }

    /// @cond
    /// Internal helper for WaitAny/WaitAll
    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept { return request_.TryGetContextAccessor(); }
    /// @endcond

private:
    RequestEvalCommon request_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
