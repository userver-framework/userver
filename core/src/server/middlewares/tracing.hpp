#pragma once

#include <userver/dynamic_config/source.hpp>
#include <userver/logging/level.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class TracingManagerBase;
}

namespace server::http {
class HttpResponse;
}

namespace server::middlewares {

class Tracing final : public HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName{"userver-tracing-middleware"};

  Tracing(const tracing::TracingManagerBase& tracing_manager,
          const handlers::HttpHandlerBase& handler);

 private:
  struct LoggingSettings final {
    bool need_log_response{false};
    bool need_log_response_headers{false};
  };

  void HandleRequest(http::HttpRequest& request,
                     request::RequestContext& context) const override;

  tracing::Span MakeSpan(const http::HttpRequest& http_request,
                         std::string_view meta_type) const;

  void FillResponseWithTracingContext(const tracing::Span& span,
                                      http::HttpResponse& http_request) const;

  LoggingSettings ParseLoggingSettings(request::RequestContext& context) const;

  void EnrichLogs(tracing::Span& span, const LoggingSettings& logging_settings,
                  const http::HttpRequest& request,
                  request::RequestContext& context) const;

  const tracing::TracingManagerBase& tracing_manager_;
  const handlers::HttpHandlerBase& handler_;
  std::optional<logging::Level> log_level_;
};

class TracingFactory final : public HttpMiddlewareFactoryBase {
 public:
  static constexpr std::string_view kName = Tracing::kName;

  TracingFactory(const components::ComponentConfig&,
                 const components::ComponentContext&);

 private:
  std::unique_ptr<HttpMiddlewareBase> Create(
      const handlers::HttpHandlerBase&, yaml_config::YamlConfig) const override;

  const tracing::TracingManagerBase& tracing_manager_;
};

}  // namespace server::middlewares

template <>
inline constexpr bool
    components::kHasValidate<server::middlewares::TracingFactory> = true;

template <>
inline constexpr auto
    components::kConfigFileMode<server::middlewares::TracingFactory> =
        ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
