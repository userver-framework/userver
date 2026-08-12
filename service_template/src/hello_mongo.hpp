// mongo template on
#pragma once

#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <userver/storages/mongo/pool.hpp>

namespace service_template {

class HelloMongo final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-hello-mongo";

    HelloMongo(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    std::string HandleRequestThrow(const userver::server::http::HttpRequest&, userver::server::request::RequestContext&)
        const override;

private:
    userver::storages::mongo::PoolPtr mongo_pool_;
};

}  // namespace service_template
// mongo template off
