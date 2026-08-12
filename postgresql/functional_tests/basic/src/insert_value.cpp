#include <insert_value.hpp>

#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>

#include <postgresql_tests_basic/sql_queries.hpp>

namespace postgresql_tests_basic {

InsertValue::InsertValue(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(context.FindComponent<components::Postgres>("key-value-database").GetCluster())
{}

std::string InsertValue::HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
    const {
    const auto& key = request.GetArg("key");
    if (key.empty()) {
        throw server::handlers::ClientError(server::handlers::ExternalBody{"No 'key' query argument"});
    }
    const auto& value = request.GetArg("value");

    pg_cluster_->Execute(storages::postgres::ClusterHostType::kMaster, sql::kInsertValue, key, value);

    request.GetHttpResponse().SetContentType(http::content_type::kTextPlain);
    request.SetResponseStatus(server::http::HttpStatus::kCreated);
    return std::string{value};
}

}  // namespace postgresql_tests_basic
