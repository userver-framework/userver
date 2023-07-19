#include <server/handlers/http_server_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

bool ParseLogRequest(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_LOG_REQUEST").As<bool>();
}

bool ParseLogRequestHeaders(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_LOG_REQUEST_HEADERS").As<bool>();
}

bool ParseCheckAuthInHandlers(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_CHECK_AUTH_IN_HANDLERS").As<bool>();
}

bool ParseCancelHandleRequestByDeadline(
    const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE").As<bool>();
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
