#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/server/handlers/http_handler_base.hpp>

namespace userver_httparena {
class DatasetProvider;

namespace json {
class Handler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "json-handler";

    Handler(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const final;

private:
    const DatasetProvider& dataset_provider_;
};
}  // namespace json
}  // namespace userver_httparena
