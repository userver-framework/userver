#pragma once

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace postgresql_tests_basic {

// GET /v1/key-value?key=... -- returns the value stored for the key.
class SelectValue final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-select-value";

    SelectValue(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override;

private:
    storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace postgresql_tests_basic
