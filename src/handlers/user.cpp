#include <string>
#include <string_view>
#include <fmt/format.h>

#include <userver/server/handlers/http_handler_base.hpp>
#include "user.h"

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>


namespace service_template {

class Profiles final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-user";

  using HttpHandlerBase::HttpHandlerBase;

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &) const override {
      
      

      return "";
  }
};

void AppendProfiles(userver::components::ComponentList &component_list) {
  component_list.Append<Profiles>();
}

} // namespace service_template
