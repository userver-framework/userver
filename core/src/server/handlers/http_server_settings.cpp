#include <server/handlers/http_server_settings.hpp>

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

/// [bool config sample]
const dynamic_config::Key<bool> kLogRequest{"USERVER_LOG_REQUEST", true};
/// [bool config sample]

const dynamic_config::Key<bool> kLogRequestHeaders{
    "USERVER_LOG_REQUEST_HEADERS", false};

const dynamic_config::Key<bool> kCancelHandleRequestByDeadline{
    "USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE", false};

CcCustomStatus Parse(const formats::json::Value& value,
                     formats::parse::To<CcCustomStatus>) {
  return CcCustomStatus{
      static_cast<http::HttpStatus>(value["initial-status-code"].As<int>(429)),
      std::chrono::milliseconds{
          value["max-time-ms"].As<std::chrono::milliseconds::rep>(10000)},
  };
}

const dynamic_config::Key<CcCustomStatus> kCcCustomStatus{
    "USERVER_RPS_CCONTROL_CUSTOM_STATUS",
    dynamic_config::DefaultAsJsonString{"{}"},
};

const dynamic_config::Key<bool> kStreamApiEnabled{
    "USERVER_HANDLER_STREAM_API_ENABLED", false};

}  // namespace server::handlers

USERVER_NAMESPACE_END
