#pragma once

/// @file
/// @brief @copybrief storages::redis::RequestGeneric

#include <userver/compiler/impl/lifetime.hpp>
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
    explicit RequestGeneric(RequestGenericCommon&& request)
        : request_(std::move(request))
    {}

    void Wait() { request_.Wait(); }

    void IgnoreResult() const { request_.IgnoreResult(); }

    ReplyType Get(const std::string& request_description = {}) {
        return impl::ParseReply<ReplyType, ReplyType>(request_.GetRaw(), request_description);
    }

    /// Satisfies @ref engine::Awaitable, for use with @ref engine::WaitAnyContext and friends.
    engine::AwaitableToken GetAwaitableToken() noexcept USERVER_IMPL_LIFETIME_BOUND {
        return request_.GetAwaitableToken();
    }

private:
    RequestGenericCommon request_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
