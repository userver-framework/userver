#include <userver/server/middlewares/http_middleware_base.hpp>

#include <utility>
#include <vector>

#include <userver/components/component_config.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/impl/validate_static_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

namespace {

const utils::AnyStorageDataTag<request::StorageContext, std::vector<HttpMiddlewareBase::PostParseCallback>>
    kPostParseCallbacks;

}  // namespace

HttpMiddlewareBase::HttpMiddlewareBase() = default;

HttpMiddlewareBase::~HttpMiddlewareBase() = default;

void HttpMiddlewareBase::Next(http::HttpRequest& request, request::RequestContext& context) const {
    UASSERT(next_);
    next_->HandleRequest(request, context);
}

void HttpMiddlewareBase::RegisterPostParseCallback(request::RequestContext& context, PostParseCallback callback) {
    auto* callbacks = context.GetDataOptional(kPostParseCallbacks);
    if (!callbacks) {
        callbacks = &context.EmplaceData(kPostParseCallbacks);
    }
    callbacks->push_back(std::move(callback));
}

void HttpMiddlewareBase::RunPostParseCallbacks(const http::HttpRequest& request, request::RequestContext& context) {
    auto* pending = context.GetDataOptional(kPostParseCallbacks);
    if (!pending) {
        return;
    }

    auto callbacks = std::move(*pending);
    context.EraseData(kPostParseCallbacks);
    for (const auto& callback : callbacks) {
        try {
            callback(request, context);
        } catch (...) {
            LOG_LIMITED_WARNING() << "HTTP post-parse callback failed";
        }
    }
}

HttpMiddlewareFactoryBase::HttpMiddlewareFactoryBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : components::ComponentBase{config, context}
{}

std::unique_ptr<HttpMiddlewareBase> HttpMiddlewareFactoryBase::CreateChecked(
    const handlers::HttpHandlerBase& handler,
    yaml_config::YamlConfig middleware_config
) const {
    if (!middleware_config.IsMissing()) {
        yaml_config::impl::Validate(middleware_config, GetMiddlewareConfigSchema());
    }

    return Create(handler, std::move(middleware_config));
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
