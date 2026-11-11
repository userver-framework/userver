#pragma once

/// @file userver/server/middlewares/http_middleware_base.hpp
/// @brief Base classes for implementing custom middlewares

#include <functional>
#include <memory>

#include <userver/components/component_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/yaml_config/schema.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {

namespace request {
class RequestContext;
}

namespace handlers {
class HttpHandlerBase;
}

/// HTTP server middlewares
namespace middlewares {

class HandlerAdapter;

/// @ingroup userver_middlewares userver_base_classes
///
/// @brief Base class for a http middleware
class HttpMiddlewareBase {
public:
    /// @brief A callback executed after request parsing by the handler adapter.
    using PostParseCallback = std::function<void(const http::HttpRequest&, request::RequestContext&)>;

    HttpMiddlewareBase();
    virtual ~HttpMiddlewareBase();

protected:
    /// @brief Override this method to implement a middleware logic
    virtual void HandleRequest(http::HttpRequest& request, request::RequestContext& context) const = 0;

    /// @brief The method to invoke the next middleware in a pipeline
    void Next(http::HttpRequest& request, request::RequestContext& context) const;

    /// @brief Register a callback before @ref server::middlewares::HttpMiddlewareBase::Next.
    /// The handler adapter calls it after successful parsing and before request handling.
    /// Callbacks run in registration order. A parsing failure or a short-circuit in the pipeline
    /// skips them. Callback exceptions are logged and do not interrupt request handling.
    static void RegisterPostParseCallback(request::RequestContext& context, PostParseCallback callback);

private:
    friend class handlers::HttpHandlerBase;
    friend class HandlerAdapter;

    static void RunPostParseCallbacks(const http::HttpRequest& request, request::RequestContext& context);

    std::unique_ptr<HttpMiddlewareBase> next_{nullptr};
};

/// @ingroup userver_middlewares userver_base_classes
///
/// @brief Base class for a http middleware-factory
class HttpMiddlewareFactoryBase : public components::ComponentBase {
public:
    HttpMiddlewareFactoryBase(const components::ComponentConfig&, const components::ComponentContext&);

    std::unique_ptr<HttpMiddlewareBase> CreateChecked(
        const handlers::HttpHandlerBase& handler,
        yaml_config::YamlConfig middleware_config
    ) const;

protected:
    /// @brief This method should return the schema of a middleware configuration,
    /// if any. You may override it
    virtual yaml_config::Schema GetMiddlewareConfigSchema() const { return yaml_config::Schema::EmptyObject(); }

    /// @brief Override this method to create an instance of a middleware
    virtual std::unique_ptr<HttpMiddlewareBase> Create(
        const handlers::HttpHandlerBase&,
        yaml_config::YamlConfig middleware_config
    ) const = 0;
};

/// @ingroup userver_middlewares
///
/// @brief A short-cut for defining a middleware-factory
template <typename Middleware>
class SimpleHttpMiddlewareFactory final : public HttpMiddlewareFactoryBase {
public:
    static constexpr std::string_view kName = Middleware::kName;

    using HttpMiddlewareFactoryBase::HttpMiddlewareFactoryBase;

private:
    std::unique_ptr<HttpMiddlewareBase> Create(const handlers::HttpHandlerBase& handler, yaml_config::YamlConfig)
        const override {
        return std::make_unique<Middleware>(handler);
    }
};

}  // namespace middlewares
}  // namespace server

template <typename Middleware>
inline constexpr bool components::kHasValidate<server::middlewares::SimpleHttpMiddlewareFactory<Middleware>> = true;

template <typename Middleware>
inline constexpr auto components::kConfigFileMode<
    server::middlewares::SimpleHttpMiddlewareFactory<Middleware>> = ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
