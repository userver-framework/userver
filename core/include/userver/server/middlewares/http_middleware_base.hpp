#pragma once

#include <memory>

#include <userver/components/loggable_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {

namespace request {
class RequestContext;
}

namespace http {
class HttpRequest;
}

namespace handlers {
class HttpHandlerBase;
}

namespace middlewares {

class HttpMiddlewareBase {
 public:
  HttpMiddlewareBase();
  virtual ~HttpMiddlewareBase();

 protected:
  virtual void HandleRequest(http::HttpRequest& request,
                             request::RequestContext& context) const = 0;

  void Next(http::HttpRequest& request, request::RequestContext& context) const;

 private:
  friend class handlers::HttpHandlerBase;

  std::unique_ptr<HttpMiddlewareBase> next_{nullptr};
};

class HttpMiddlewareFactoryBase : public components::LoggableComponentBase {
 public:
  HttpMiddlewareFactoryBase(const components::ComponentConfig&,
                            const components::ComponentContext&);

  virtual std::unique_ptr<HttpMiddlewareBase> Create(
      const handlers::HttpHandlerBase&,
      // TODO : TAXICOMMON-8253, more concise config
      const components::ComponentConfig&) const = 0;
};

template <typename Middleware>
class SimpleHttpMiddlewareFactory final : public HttpMiddlewareFactoryBase {
 public:
  static constexpr std::string_view kName = Middleware::kName;

  using HttpMiddlewareFactoryBase::HttpMiddlewareFactoryBase;

 private:
  std::unique_ptr<HttpMiddlewareBase> Create(
      const handlers::HttpHandlerBase& handler,
      const components::ComponentConfig& handler_config) const override {
    return std::make_unique<Middleware>(handler, handler_config);
  }
};

}  // namespace middlewares
}  // namespace server

template <typename Middleware>
inline constexpr bool components::kHasValidate<
    server::middlewares::SimpleHttpMiddlewareFactory<Middleware>> = true;

template <typename Middleware>
inline constexpr auto components::kConfigFileMode<
    server::middlewares::SimpleHttpMiddlewareFactory<Middleware>> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
