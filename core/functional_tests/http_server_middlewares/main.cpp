#include <fmt/format.h>

#include <userver/components/component_config.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/middlewares/configuration.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

namespace {

constexpr std::string_view kOrderHeader = "X-Middleware-Order";

class RecordingMiddleware final : public server::middlewares::HttpMiddlewareBase {
public:
    explicit RecordingMiddleware(std::string name)
        : name_(std::move(name))
    {}

private:
    void HandleRequest(server::http::HttpRequest& request, server::request::RequestContext& context) const override {
        auto& response = request.GetHttpResponse();
        response.SetHeader(kOrderHeader, fmt::format("{}{},", response.GetHeader(kOrderHeader), name_));
        Next(request, context);
    }

    const std::string name_;
};

class RecordingMiddlewareFactory final : public server::middlewares::HttpMiddlewareFactoryBase {
public:
    RecordingMiddlewareFactory(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpMiddlewareFactoryBase(config, context),
          name_(config.Name())
    {}

private:
    std::unique_ptr<server::middlewares::HttpMiddlewareBase> Create(
        const server::handlers::HttpHandlerBase&,
        yaml_config::YamlConfig
    ) const override {
        return std::make_unique<RecordingMiddleware>(name_);
    }

    const std::string name_;
};

class OrderHandler final : public server::handlers::HttpHandlerBase {
public:
    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequest(server::http::HttpRequest&, server::request::RequestContext&) const override {
        return "handled";
    }
};

}  // namespace

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<server::middlewares::HandlerPipelineBuilder>("server-only-pipeline")
            .Append<OrderHandler>("handler-server-only")
            .Append<OrderHandler>("handler-combined")
            .Append<RecordingMiddlewareFactory>("server-prepend-first")
            .Append<RecordingMiddlewareFactory>("server-prepend-second")
            .Append<RecordingMiddlewareFactory>("server-append-first")
            .Append<RecordingMiddlewareFactory>("server-append-second")
            .Append<RecordingMiddlewareFactory>("handler-prepend-first")
            .Append<RecordingMiddlewareFactory>("handler-prepend-second")
            .Append<RecordingMiddlewareFactory>("handler-append-first")
            .Append<RecordingMiddlewareFactory>("handler-append-second");
    return utils::DaemonMain(argc, argv, component_list);
}
