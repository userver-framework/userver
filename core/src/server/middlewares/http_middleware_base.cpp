#include <userver/server/middlewares/http_middleware_base.hpp>

#include <userver/components/component_config.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/impl/validate_static_config.hpp>

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

std::unique_ptr<HttpMiddlewareBase> HttpMiddlewareFactoryBase::CreateChecked(
    const handlers::HttpHandlerBase& handler,
    yaml_config::YamlConfig middleware_config) const {
  if (!middleware_config.IsMissing()) {
    yaml_config::impl::Validate(middleware_config, GetMiddlewareConfigSchema());
  }

  return Create(handler, std::move(middleware_config));
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
