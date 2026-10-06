#pragma once
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/request/client_metrics_shard.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace handler {

class HandlerVeryImportantProductUpstream : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-very-important-product-upstream";

    HandlerVeryImportantProductUpstream(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    )
        : HttpHandlerBase(config, context),
          upstream_url_{config["upstream-url"].As<std::string>()},
          http_client_(context.FindComponent<components::HttpClient>().GetHttpClient())
    {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        if (request.HasArg("product_slice")) {
            server::request::SetClientMetricsShard(
                "product_sliced",
                {utils::statistics::LabelView{"product_slice", request.GetArg("product_slice")}}
            );
        }

        auto response = http_client_.CreateRequest().get(upstream_url_).perform();
        response->raise_for_status();
        return "OK!";
    }

    static yaml_config::Schema GetStaticConfigSchema() {
        return yaml_config::MergeSchemas<server::handlers::HttpHandlerBase>(R"(
type: object
description: Handler that requests an upstream service
additionalProperties: false
properties:
    upstream-url:
        type: string
        description: URL of the upstream service
)");
    }

private:
    const std::string upstream_url_;
    clients::http::Client& http_client_;
};

}  // namespace handler
