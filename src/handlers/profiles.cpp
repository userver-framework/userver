#include <string>
#include <string_view>
#include <fmt/format.h>

#include <userver/server/handlers/http_handler_base.hpp>
#include "profiles.hpp"

namespace real_medium {

class Profiles final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-profiles";

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
