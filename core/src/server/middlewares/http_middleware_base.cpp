#include <userver/server/middlewares/http_middleware_base.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

HttpMiddlewareBase::HttpMiddlewareBase() = default;

HttpMiddlewareBase::~HttpMiddlewareBase() = default;

void HttpMiddlewareBase::Next(http::HttpRequest& request,
                              request::RequestContext& context) const {
  UASSERT(next_);
  next_->HandleRequest(request, context);
}

HttpMiddlewareFactoryBase::HttpMiddlewareFactoryBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase{config, context} {}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
