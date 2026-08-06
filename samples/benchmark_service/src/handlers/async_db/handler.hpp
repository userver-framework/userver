#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/postgres/postgres_fwd.hpp>

namespace userver_httparena::async_db {
class Handler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "async-db-handler";

    Handler(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const final;

private:
    const storages::postgres::ClusterPtr pg_;
};
}  // namespace userver_httparena::async_db
