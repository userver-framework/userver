#include <select_value.hpp>

#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>

#include <postgresql_tests_basic/sql_queries.hpp>

namespace postgresql_tests_basic {

SelectValue::SelectValue(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(context.FindComponent<components::Postgres>("key-value-database").GetCluster())
{}

std::string SelectValue::HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
    const {
    const auto& key = request.GetArg("key");
    if (key.empty()) {
        throw server::handlers::ClientError(server::handlers::ExternalBody{"No 'key' query argument"});
    }

    const auto res = pg_cluster_->Execute(storages::postgres::ClusterHostType::kSlave, sql::kSelectValue, key);

    request.GetHttpResponse().SetContentType(http::content_type::kTextPlain);
    if (res.IsEmpty()) {
        request.SetResponseStatus(server::http::HttpStatus::kNotFound);
        return {};
    }

    return res.AsSingleRow<std::string>();
}

}  // namespace postgresql_tests_basic
