#pragma once

/// @file
/// @brief @copybrief storages::redis::RequestGeneric

#include <userver/storages/redis/parse_reply.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/storages/redis/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief Redis future for generic command responses.
/// Can be used to request custom modules commands or unsupported yet commands
template <typename ReplyType>
class [[nodiscard]] RequestGeneric final {
public:
    explicit RequestGeneric(RequestGenericCommon&& request) : request_(std::move(request)) {}

    void Wait() { request_.Wait(); }

    void IgnoreResult() const { request_.IgnoreResult(); }

    ReplyType Get(const std::string& request_description = {}) {
        return impl::ParseReply<ReplyType, ReplyType>(request_.GetRaw(), request_description);
    }

    /// @cond
    /// Internal helper for WaitAny/WaitAll
    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept { return request_.TryGetContextAccessor(); }
    /// @endcond

private:
    RequestGenericCommon request_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
