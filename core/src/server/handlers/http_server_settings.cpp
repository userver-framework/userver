#include <server/handlers/http_server_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

const std::string kLogRequest = "USERVER_LOG_REQUEST";
const std::string kLogRequestHeaders = "USERVER_LOG_REQUEST_HEADERS";
const std::string kCheckAuthInHandlers = "USERVER_CHECK_AUTH_IN_HANDLERS";
const std::string kCancelHandleRequestByDeadline =
    "USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE";

}  // namespace

HttpServerSettings HttpServerSettings::Parse(
    const dynamic_config::DocsMap& docs_map) {
  HttpServerSettings result{};
  result.need_log_request = docs_map.Get(kLogRequest).As<bool>();
  result.need_log_request_headers = docs_map.Get(kLogRequestHeaders).As<bool>();
  result.need_check_auth_in_handlers =
      docs_map.Get(kCheckAuthInHandlers).As<bool>();
  result.need_cancel_handle_request_by_deadline =
      docs_map.Get(kCancelHandleRequestByDeadline).As<bool>();
  return result;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
