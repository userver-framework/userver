#include "tags.hpp"

#include "userver/components/component.hpp"
#include "userver/server/handlers/http_handler_json_base.hpp"
#include "userver/storages/postgres/cluster.hpp"
#include "userver/storages/postgres/component.hpp"

namespace real_medium::handlers::tags::get {

Handler::Handler(const userver::components::ComponentConfig& config,
       const userver::components::ComponentContext& component_context)
      : HttpHandlerJsonBase(config, component_context),
        pg_cluster_(component_context
                        .FindComponent<userver::components::Postgres>(
                            "realworld-database")
                        .GetCluster()) {}

  userver::formats::json::Value Handler::HandleRequestJsonThrow(
      const userver::server::http::HttpRequest&,
      const userver::formats::json::Value&,
      userver::server::request::RequestContext&) const {
    constexpr static auto query = "SELECT tag_name FROM realworld.tag_list";
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave, query);
    auto tags = result.AsSetOf<std::string>();

    userver::formats::json::ValueBuilder response;
    response["tags"] = tags;

    return response.ExtractValue();
  }


} // namespace real_medium::handlers::tags::get