#include <string>

#include "users.hpp"
#include "userver/server/handlers/http_handler_base.hpp"

#include "userver/storages/postgres/component.hpp"

#include "userver/components/component_config.hpp"
#include "userver/components/component_context.hpp"

namespace real_medium::handlers::users::post {

Handler::Handler(const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& component_context)
      : HttpHandlerBase(config, component_context)
      , pg_cluster_(component_context
                        .FindComponent<userver::components::Postgres>(
                            "realworld-database")
                        .GetCluster()) {}

  std::string Handler::HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const {
    return "";
  }



} // namespace real_medium::handlers::users::post
