#include <clients/http/middlewares/pipeline.hpp>

#include <boost/range/adaptor/reversed.hpp>

#include <clients/http/request_state.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

MiddlewaresPipeline::MiddlewaresPipeline(utils::span<const utils::NotNull<MiddlewareBase*>> middlewares)
    : middlewares_(middlewares)
{}

void MiddlewaresPipeline::HookCreateSpan(RequestState& request_state, tracing::Span& span) {
    MiddlewareRequest req(request_state);

    for (const auto& middleware : middlewares_) {
        middleware->HookCreateSpan(req, span);
    }
}

void MiddlewaresPipeline::HookOnCompleted(RequestState& request_state, Response& response) {
    MiddlewareRequest req(request_state);

    // NOLINTNEXTLINE(modernize-loop-convert)
    for (const auto& middleware : middlewares_ | boost::adaptors::reversed) {
        middleware->HookOnCompleted(req, response);
    }
}

void MiddlewaresPipeline::HookOnError(RequestState& request_state, std::error_code ec) {
    MiddlewareRequest req(request_state);

    // NOLINTNEXTLINE(modernize-loop-convert)
    for (const auto& middleware : middlewares_ | boost::adaptors::reversed) {
        middleware->HookOnError(req, ec);
    }
}

bool MiddlewaresPipeline::HookOnRetry(RequestState& request_state) {
    MiddlewareRequest req(request_state);

    for (const auto& middleware : middlewares_) {
        if (!middleware->HookOnRetry(req)) {
            return false;
        }
    }
    return true;
}

void MiddlewaresPipeline::HookPerformRequest(RequestState& request_state) {
    MiddlewareRequest req(request_state);

    for (const auto& middleware : middlewares_) {
        middleware->HookPerformRequest(req);
    }
}

}  // namespace clients::http

USERVER_NAMESPACE_END
