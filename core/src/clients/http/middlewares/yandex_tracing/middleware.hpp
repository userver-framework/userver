#pragma once

#include <userver/clients/http/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares::yandex_tracing {

class Middleware final : public http::MiddlewareBase {
public:
    void HookCreateSpan(MiddlewareRequest& request, tracing::Span& span) override;

    void HookOnCompleted(MiddlewareRequest& request, Response& response) override;
};

}  // namespace clients::http::middlewares::yandex_tracing

USERVER_NAMESPACE_END
