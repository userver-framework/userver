#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
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

class SomeServerMiddleware final
    : public server::middlewares::HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName{"server-middleware"};

  // Neither handler nor its config is interesting to us,
  // but we could use it if needed
  SomeServerMiddleware(const server::handlers::HttpHandlerBase&,
                       const components::ComponentConfig&) {}

 private:
  void HandleRequest(server::http::HttpRequest& request,
                     server::request::RequestContext& context) const override {
    Next(request, context);

    // In case the header is needed even in a presence of downstream exceptions,
    // this should be wrapped in a scope guard (utils::ScopeGuard, for example)
    request.GetHttpResponse().SetHeader(kCustomServerHeader, "1");
  }

  static constexpr http::headers::PredefinedHeader kCustomServerHeader{
      "X-Some-Server-Header"};
};
using SomeServerMiddlewareFactory =
    server::middlewares::SimpleHttpMiddlewareFactory<SomeServerMiddleware>;

class SomeHandlerMiddleware final
    : public server::middlewares::HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName{"handler-middleware"};

  // Neither handler nor its config is interesting to us,
  // but we could use it if needed
  SomeHandlerMiddleware(const server::handlers::HttpHandlerBase&,
                        const components::ComponentConfig&) {}

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
    const utils::ScopeGuard set_header_scope{[&request] {
      request.GetHttpResponse().SetHeader(kCustomHandlerHeader, "1");
    }};

    Next(request, context);
  }

  static constexpr http::headers::PredefinedHeader kCustomHandlerHeader{
      "X-Some-Handler-Header"};
};
using SomeHandlerMiddlewareFactory =
    server::middlewares::SimpleHttpMiddlewareFactory<SomeHandlerMiddleware>;

class CustomHandlerPipelineBuilder final
    : public server::middlewares::HandlerPipelineBuilder {
 public:
  using HandlerPipelineBuilder::HandlerPipelineBuilder;

  server::middlewares::MiddlewaresList BuildPipeline(
      server::middlewares::MiddlewaresList pipeline) const override {
    // We could do any kind of transformation here.
    // For the sake of example (and what we assume to be the most common case),
    // we just add one more middleware to the pipeline.
    pipeline.emplace_back(SomeHandlerMiddleware::kName);

    return pipeline;
  }
};

}  // namespace samples::http_middlewares

template <>
constexpr auto components::kConfigFileMode<
    samples::http_middlewares::CustomHandlerPipelineBuilder> =
    components::ConfigFileMode::kNotRequired;

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          // handlers
          .Append<samples::http_middlewares::Handler>("handler")
          .Append<samples::http_middlewares::Handler>(
              "handler-with-custom-middlewares")
          // middlewares
          .Append<samples::http_middlewares::SomeServerMiddlewareFactory>()
          .Append<samples::http_middlewares::SomeHandlerMiddlewareFactory>()
          // configuration
          .Append<samples::http_middlewares::CustomHandlerPipelineBuilder>(
              "custom-handler-pipeline-builder");

  return utils::DaemonMain(argc, argv, component_list);
}
