#pragma once
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

namespace handler {

class HandlerVeryImportantProduct : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-very-important-product";

    HandlerVeryImportantProduct(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context)
    {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext& context)
        const override {
        if (request.HasArg("product_slice")) {
            const auto& product_slice_designator = request.GetArg("product_slice");
            // Register new sensor sub-path "http.handler.product_sliced.*"
            // And provide each metric with label
            context.SetHandlerMetricsShard(
                "product_sliced",
                {utils::statistics::LabelView{"product_slice", product_slice_designator}}
            );
        }

        return "OK!";
    }
};

}  // namespace handler
