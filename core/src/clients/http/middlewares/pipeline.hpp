#pragma once

#include <userver/clients/http/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class MiddlewaresPipeline final {
public:
    MiddlewaresPipeline() = default;
    explicit MiddlewaresPipeline(utils::span<const utils::NotNull<MiddlewareBase*>> middlewares);

    void HookPerformRequest(RequestState& request);

    void HookCreateSpan(RequestState& request, tracing::Span& span);

    void HookOnCompleted(RequestState& request, Response& response);

    void HookOnError(RequestState& request, std::error_code ec);

    bool HookOnRetry(RequestState& request);

private:
    utils::span<const utils::NotNull<MiddlewareBase*>> middlewares_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
