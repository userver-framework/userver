#pragma once

#include <userver/clients/http/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares::headers_propagator {

class Middleware final : public http::MiddlewareBase {
public:
    void HookCreateSpan(MiddlewareRequest&, tracing::Span& span) override;
};

}  // namespace clients::http::middlewares::headers_propagator

USERVER_NAMESPACE_END
