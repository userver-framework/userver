#include <string>
#include <string_view>
#include <fmt/format.h>

#include "profiles.hpp"
#include "userver/server/handlers/http_handler_base.hpp"

namespace real_medium::handlers::profiles::get {

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

} // namespace real_medium::handlers::profiles::get
