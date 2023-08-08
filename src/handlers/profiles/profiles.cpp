#include <string>
#include <string_view>
#include <fmt/format.h>

#include "profiles.hpp"
#include "userver/server/handlers/http_handler_base.hpp"

namespace real_medium::handlers::profiles::get {

std::string Handler::HandleRequestThrow(
    const userver::server::http::HttpRequest &request,
    userver::server::request::RequestContext &) const {
  return "";
}

} // namespace real_medium::handlers::profiles::get
