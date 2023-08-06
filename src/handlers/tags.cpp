#include "tags.hpp"

#include <userver/components/component.hpp>
#include <userver/formats/json/serialize_container.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace real_medium {

class Tags final : public userver::server::handlers::HttpHandlerJsonBase {
  userver::storages::postgres::ClusterPtr pg_cluster_;

 public:
  static constexpr std::string_view kName = "handler-get-tags";

  Tags(const userver::components::ComponentConfig& config,
          const userver::components::ComponentContext& component_context)
      : HttpHandlerJsonBase(config, component_context),
        pg_cluster_(component_context
                        .FindComponent<userver::components::Postgres>(
                            "realworld-database")
                        .GetCluster()) {}

  userver::formats::json::Value HandleRequestJsonThrow(
      const userver::server::http::HttpRequest&,
      const userver::formats::json::Value&,
      userver::server::request::RequestContext&) const override {
    constexpr static auto query = "SELECT tag_name FROM realworld.tag_list";
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kSlave, query);
    auto tags = result.AsSetOf<std::string>();

    userver::formats::json::ValueBuilder response;
    response["tags"] = tags;

    return response.ExtractValue();
  }
};

void AppendGetTags(userver::components::ComponentList& component_list) {
  component_list.Append<Tags>();
}

}  // namespace real_medium