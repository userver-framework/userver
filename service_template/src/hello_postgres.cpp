// postgresql template on
#include <hello_postgres.hpp>

#include <greeting.hpp>

#include <userver/storages/postgres/component.hpp>

namespace service_template {

HelloPostgres::HelloPostgres(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context
)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context.FindComponent<userver::components::Postgres>("postgres-db-1").GetCluster()) {}

std::string HelloPostgres::
    HandleRequestThrow(const userver::server::http::HttpRequest& request, userver::server::request::RequestContext&)
        const {
    const auto& name = request.GetArg("name");
    auto user_type = UserType::kFirstTime;

    if (!name.empty()) {
        const auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO hello_schema.users(name, count) VALUES($1, 1) "
            "ON CONFLICT (name) "
            "DO UPDATE SET count = users.count + 1 "
            "RETURNING users.count",
            name
        );

        if (result.AsSingleRow<int>() > 1) {
            user_type = UserType::kKnown;
        }
    }

    return SayHelloTo(name, user_type);
}

}  // namespace service_template
// postgresql template off
