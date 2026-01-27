// postgresql template on
#pragma once

#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <userver/storages/postgres/cluster.hpp>

namespace service_template {

class HelloPostgres final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-hello-postgres";

    HelloPostgres(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    std::string HandleRequestThrow(const userver::server::http::HttpRequest&, userver::server::request::RequestContext&)
        const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace service_template
// postgresql template off
