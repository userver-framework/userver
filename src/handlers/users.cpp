#include <fmt/format.h>
#include <string>
#include <string_view>

#include <userver/server/handlers/http_handler_base.hpp>
#include "users.h"

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>


namespace service_template {

class Users final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-users";


    checkerItem(const userver::components::ComponentConfig& config,
                const userver::components::ComponentContext& component_context)
                : HttpHandlerBase(config, component_context),
                pg_cluster_(
                    component_context
                      .FindComponent<userver::components::Postgres>("postgres-db-1")
                      .GetCluster()) {}


  using HttpHandlerBase::HttpHandlerBase;
  
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override {
    return "";
  }
};

void AppendUsers(userver::components::ComponentList& component_list) {
  component_list.Append<Users>();
}

}  // namespace service_template
