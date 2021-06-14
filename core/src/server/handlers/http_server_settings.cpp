#include <server/handlers/http_server_settings.hpp>

namespace server::handlers {

namespace {

const std::string kLogRequest = "USERVER_LOG_REQUEST";
const std::string kLogRequestHeaders = "USERVER_LOG_REQUEST_HEADERS";
const std::string kCheckAuthInHandlers = "USERVER_CHECK_AUTH_IN_HANDLERS";

}  // namespace

HttpServerSettings HttpServerSettings::Parse(
    const taxi_config::DocsMap& docs_map) {
  HttpServerSettings result{};
  result.need_log_request = docs_map.Get(kLogRequest).As<bool>();
  result.need_log_request_headers = docs_map.Get(kLogRequestHeaders).As<bool>();
  result.need_check_auth_in_handlers =
      docs_map.Get(kCheckAuthInHandlers).As<bool>();
  return result;
}

}  // namespace server::handlers
