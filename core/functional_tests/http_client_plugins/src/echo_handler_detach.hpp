#pragma once

#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

class EchoHandlerDetach final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-echo-detach";

    EchoHandlerDetach(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          echo_url_{config["echo-url"].As<std::string>()},
          http_client_(context.FindComponent<components::HttpClient>().GetHttpClient())
    {}

    std::string HandleRequestThrow(const server::http::HttpRequest&, server::request::RequestContext&) const override {
        http_client_.CreateRequest().get(echo_url_).async_perform().Detach();
        return {};
    }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<server::handlers::HttpHandlerBase>(R"(
    type: object
    description: HTTP echo detach component
    additionalProperties: false
    properties:
        echo-url:
            type: string
            description: some other microservice listens on this URL
    )");
    }

private:
    const std::string echo_url_;
    clients::http::Client& http_client_;
};
