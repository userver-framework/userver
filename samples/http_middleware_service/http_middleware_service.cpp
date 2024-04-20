#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utils/scope_guard.hpp>

#include <userver/server/middlewares/configuration.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>

namespace samples::http_middlewares {

class Handler final : public server::handlers::HttpHandlerBase {
 public:
  using HttpHandlerBase::HttpHandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest&,
      server::request::RequestContext&) const override {
    return "Hello world!\n";
  }
};

/// [Middlewares sample - minimal implementation]
class NoopMiddleware final : public server::middlewares::HttpMiddlewareBase {
 private:
  void HandleRequest(server::http::HttpRequest& request,
                     server::request::RequestContext& context) const override {
    Next(request, context);
  }
};
/// [Middlewares sample - minimal implementation]

/// [Middlewares sample - minimal factory implementation]
class NoopMiddlewareFactory final
    : public server::middlewares::HttpMiddlewareFactoryBase {
 public:
  static constexpr std::string_view kName{"noop-middleware"};

  using HttpMiddlewareFactoryBase::HttpMiddlewareFactoryBase;

 private:
  std::unique_ptr<server::middlewares::HttpMiddlewareBase> Create(
      const server::handlers::HttpHandlerBase&,
      yaml_config::YamlConfig) const override {
    return std::make_unique<NoopMiddleware>();
  }
};
/// [Middlewares sample - minimal factory implementation]

/// [Middlewares sample - some middleware implementation]
class SomeServerMiddleware final
    : public server::middlewares::HttpMiddlewareBase {
 public:
  // This will be used as a kName for the SimpleHttpMiddlewareFactory
  static constexpr std::string_view kName{"server-middleware"};

  // Handler isn't interesting to us, but we could use it if needed.
  // Or we could implement the factory ourselves instead of using
  // SimpleHttpMiddlewareFactory, and pass whatever parameters we want.
  explicit SomeServerMiddleware(const server::handlers::HttpHandlerBase&) {}

 private:
  void HandleRequest(server::http::HttpRequest& request,
                     server::request::RequestContext& context) const override {
    Next(request, context);

    request.GetHttpResponse().SetHeader(kCustomServerHeader, "1");
  }

  static constexpr http::headers::PredefinedHeader kCustomServerHeader{
      "X-Some-Server-Header"};
};
/// [Middlewares sample - some middleware implementation]
/// [Middlewares sample - some middleware factory implementation]
using SomeServerMiddlewareFactory =
    server::middlewares::SimpleHttpMiddlewareFactory<SomeServerMiddleware>;
/// [Middlewares sample - some middleware factory implementation]

/// [Middlewares sample - configurable middleware implementation]
class SomeHandlerMiddleware final
    : public server::middlewares::HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName{"handler-middleware"};

  SomeHandlerMiddleware(const server::handlers::HttpHandlerBase&,
                        yaml_config::YamlConfig middleware_config)
      : header_value_{middleware_config["header-value"].As<std::string>()} {}

 private:
  void HandleRequest(server::http::HttpRequest& request,
                     server::request::RequestContext& context) const override {
    // We will set the header no matter whether downstream succeeded or not.
    //
    // Note that in presence of exceptions other that
    // CustomHandlerException-derived ones, this header would be removed by
    // default userver-provided exceptions handling middleware. If this is
    // undesirable, the middleware should be earlier in the pipeline, or the
    // default exceptions handling behavior could be overridden.
    const utils::ScopeGuard set_header_scope{[this, &request] {
      request.GetHttpResponse().SetHeader(kCustomHandlerHeader, header_value_);
    }};

    Next(request, context);
  }

  static constexpr http::headers::PredefinedHeader kCustomHandlerHeader{
      "X-Some-Handler-Header"};

  const std::string header_value_;
};
/// [Middlewares sample - configurable middleware implementation]

/// [Middlewares sample - configurable middleware factory implementation]
class SomeHandlerMiddlewareFactory final
    : public server::middlewares::HttpMiddlewareFactoryBase {
 public:
  static constexpr std::string_view kName{SomeHandlerMiddleware::kName};

  using HttpMiddlewareFactoryBase::HttpMiddlewareFactoryBase;

 private:
  std::unique_ptr<server::middlewares::HttpMiddlewareBase> Create(
      const server::handlers::HttpHandlerBase& handler,
      yaml_config::YamlConfig middleware_config) const override {
    return std::make_unique<SomeHandlerMiddleware>(
        handler, std::move(middleware_config));
  }

  yaml_config::Schema GetMiddlewareConfigSchema() const override {
    return formats::yaml::FromString(R"(
type: object
description: Config for this particular middleware
additionalProperties: false
properties:
    header-value:
        type: string
        description: header value to set for responses
)")
        .As<yaml_config::Schema>();
  }
};
/// [Middlewares sample - configurable middleware factory implementation]

/// [Middlewares sample - custom handler pipeline builder]
class CustomHandlerPipelineBuilder final
    : public server::middlewares::HandlerPipelineBuilder {
 public:
  using HandlerPipelineBuilder::HandlerPipelineBuilder;

  server::middlewares::MiddlewaresList BuildPipeline(
      server::middlewares::MiddlewaresList server_middleware_pipeline)
      const override {
    // We could do any kind of transformation here.
    // For the sake of example (and what we assume to be the most common case),
    // we just add some middleware to the pipeline.
    auto& pipeline = server_middleware_pipeline;
    pipeline.emplace_back(SomeHandlerMiddleware::kName);
    pipeline.emplace_back(NoopMiddlewareFactory::kName);

    return pipeline;
  }
};
/// [Middlewares sample - custom handler pipeline builder]

}  // namespace samples::http_middlewares

template <>
constexpr auto components::kConfigFileMode<
    samples::http_middlewares::CustomHandlerPipelineBuilder> =
    components::ConfigFileMode::kNotRequired;

template <>
constexpr auto components::kConfigFileMode<
    samples::http_middlewares::SomeHandlerMiddlewareFactory> =
    components::ConfigFileMode::kNotRequired;

template <>
constexpr auto components::kConfigFileMode<
    samples::http_middlewares::NoopMiddlewareFactory> =
    components::ConfigFileMode::kNotRequired;

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          // handlers
          .Append<samples::http_middlewares::Handler>("handler")
          .Append<samples::http_middlewares::Handler>(
              "handler-with-custom-middlewares")
          .Append<samples::http_middlewares::Handler>(
              "handler-with-another-middleware-configuration")
          // middlewares
          // clang-format off
/// [Middlewares sample - custom handler pipeline builder registration]
          .Append<samples::http_middlewares::CustomHandlerPipelineBuilder>(
              "custom-handler-pipeline-builder")
/// [Middlewares sample - custom handler pipeline builder registration]
          // clang-format on
          .Append<samples::http_middlewares::SomeServerMiddlewareFactory>()
          .Append<samples::http_middlewares::SomeHandlerMiddlewareFactory>()
          .Append<samples::http_middlewares::NoopMiddlewareFactory>();

  return utils::DaemonMain(argc, argv, component_list);
}
